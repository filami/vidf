#include "pch.h"
#include "assetbrowser.h"
#include <QFileSystemModel>
#include <QSplitter>
#include <QLineEdit>
#include <yasli/STL.h>
#include <viewport/dx11viewport.h>



//////////////////////////////////////////////////////////////////////////



void AssetItem::SortChilden(bool recursive)
{
	std::sort(
		children.begin(), children.end(),
		[](AssetItemPtr left, AssetItemPtr right)
	{
		if (left->asset != nullptr && right->asset == nullptr)
			return false;
		if (left->asset == nullptr && right->asset != nullptr)
			return true;
		return left->name < right->name;
	});
	for (uint i = 0; i < children.size(); ++i)
		children[i]->row = i;
	if (recursive)
	{
		for (auto child : children)
			child->SortChilden(true);
	}
}



QString AssetItem::GetFullName() const
{
	auto parentPtr = parent.lock();
	if (!parentPtr)
		return QString();
	return parentPtr->GetFullName() + QString("/") + QString(name.c_str());
}



QString AssetItem::GetFullTypeName() const
{
	return QString(asset->GetAssetTraits()->GetFullTypeName().c_str());
}



void AssetItem::ChangeName(const QString& _name)
{
	name = _name.toUtf8();
	if (asset)
		name = GetFullName().toUtf8();
}



AssetBrowserModel::AssetBrowserModel()
{
}



QVariant AssetBrowserModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
		return QVariant();

	AssetItem* item = GetItem(index);

	switch (role)
	{
	case Qt::DecorationRole:
		if (index.column() == 0)
		{
			if (!item->asset)
				return iconsProvider.icon(QFileIconProvider::Folder);
			else
				return iconsProvider.icon(QFileIconProvider::File);
		}
	case Qt::EditRole:
	case Qt::DisplayRole:
		if (item->asset == nullptr)
		{
			if (index.column() == 0)
				return QString(item->name.c_str());
			else
				return QString();
		}
		switch (index.column())
		{
		case 0: return QString(item->name.c_str());
		case 1: return QString(item->GetFullTypeName());
		case 2: return item->asset->GetAssetId();
		default: return QString();
		}
	default:
		return QVariant();
	}
}



bool AssetBrowserModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (index.isValid() && role == Qt::EditRole)
	{
		AssetItem* item = GetItem(index);
		AssetItem* parentItem = item->parent.lock().get();
		item->ChangeName(value.toString());
		item->parent.lock()->SortChilden(false);
		dataChanged(
			createIndex(0, index.column(), parentItem->children.front().get()),
			createIndex(parentItem->children.size()-1, index.column(), parentItem->children.back().get()));
		return true;
	}
	return false;
}



Qt::ItemFlags AssetBrowserModel::flags(const QModelIndex& index) const
{
	if (!index.isValid() || index.column() != 0)
		return Qt::ItemIsEnabled;
	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}



QVariant AssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return QVariant();
	switch (section)
	{
	case 0: return QString("Name");
	case 1: return QString("Type");
	case 2: return QString("Id");
	default: return QString();
	}
}



QModelIndex AssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
		return QModelIndex();

	AssetItem* parentItem;

	if (!parent.isValid())
		parentItem = root;
	else
		parentItem = GetItem(parent);

	AssetItemPtr childItem = parentItem->children[row];
	if (foldersOnly && childItem->asset != nullptr)
		return QModelIndex();

	return createIndex(row, column, childItem.get());
}



QModelIndex AssetBrowserModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();

	AssetItem* childItem = GetItem(index);
	AssetItemPtr parentItem = childItem->parent.lock();

	if (parentItem.get() == root)
		return QModelIndex();

	return createIndex(parentItem->row, 0, parentItem.get());
}



int AssetBrowserModel::rowCount(const QModelIndex& parent) const
{
	AssetItem* parentItem;

	if (!parent.isValid())
		parentItem = root;
	else
		parentItem = GetItem(parent);

	if (foldersOnly)
	{
		for (auto it : parentItem->children)
		{
			if (it->asset == nullptr)
				return parentItem->children.size();
		}
		return 0;
	}
	if (!recursive && parentItem != root)
		return 0;
	return parentItem->children.size();
}



int AssetBrowserModel::columnCount(const QModelIndex& parent) const
{
	if (foldersOnly)
		return 1;
	return 3;
}



AssetItem* AssetBrowserModel::GetItem(const QModelIndex& index) const
{
	return reinterpret_cast<AssetItem*>(index.internalPointer());
}



void AssetItemManager::Update()
{
	for (const auto& it : *assetManager)
	{
		AssetPtr asset = it.second;
		InsertItem(root, asset->GetName(), asset);
	}
	const bool recursive = true;
	root->SortChilden(recursive);
}



void AssetItemManager::InsertItem(AssetItemPtr parent, const std::string& name, AssetPtr asset)
{
	assert(name[0] != '/'); // ?? no name?
	size_t offset = name.find('/');
	if (offset == size_t(-1))
	{
		AssetItemPtr subItem = std::make_shared<AssetItem>();
		subItem->parent = parent;
		subItem->name = name;
		subItem->asset = asset;
		subItem->row = parent->children.size();
		parent->children.push_back(subItem);
	}
	else
	{
		std::string thisName = name.substr(0, offset);
		std::string restName = name.substr(offset + 1, name.size() - offset + 1);

		auto it = std::find_if(
			parent->children.begin(), parent->children.end(),
			[thisName](AssetItemPtr child)
		{
			return child->name == thisName;
		});
		AssetItemPtr subItem;
		if (it == parent->children.end())
		{
			subItem = std::make_shared<AssetItem>();
			subItem->parent = parent;
			subItem->name = thisName;
			subItem->row = parent->children.size();
			parent->children.push_back(subItem);
		}
		else
			subItem = *it;
		assert(subItem->asset == nullptr);		// sub item has to be a folder and not anohter real asset
		InsertItem(subItem, restName, asset);
	}
}



AssetBrowser::AssetBrowser(MainFrame& _mainFrame)
	: QDockWidget(&_mainFrame)
	, mainFrame(_mainFrame)
{
	class ShaderGraphAsset : public Asset
	{
	};
	class ShaderGraphAssetType : public AssetTraits
	{
		AssetPtr    Create() const override { return std::make_shared<ShaderGraphAsset>(); }
		const char* GetTypeName() const override { return "ShaderGraph"; }
	};
	class VoxelPaletteAssetType : public AssetTraits
	{
		AssetPtr    Create() const override { return std::make_shared<VoxelPaletteAsset>(); }
		const char* GetTypeName() const override { return "VoxelPalette"; }
	};

	auto& assetManager = GetAssetManager();
	auto& assetItemManager = GetAssetItemManager();
	assetManager.RegisterType(std::make_shared<ShaderGraphAssetType>());
	assetManager.RegisterType(std::make_shared<VoxelPaletteAssetType>());
	AssetTraitsPtr textureType = assetManager.FindTypeTraits("Texture");
	AssetTraitsPtr textureSettingsType = assetManager.FindTypeTraits("TextureSettings");
	AssetTraitsPtr shaderGraphType = assetManager.FindTypeTraits("ShaderGraph");
	AssetTraitsPtr voxelPaletteType = assetManager.FindTypeTraits("VoxelPalette");

	assetManager.MakeAsset(*textureSettingsType, "System/UncompressedNormal");
	assetManager.MakeAsset(*textureSettingsType, "System/CompressedDiffuse");
	assetManager.MakeAsset(*textureSettingsType, "System/CompressedHDR");

	assetManager.MakeAsset(*textureType, "parent 0/child 0-0");
	assetManager.MakeAsset(*textureType, "parent 1/child 1-0");
	assetManager.MakeAsset(*textureType, "parent 1/child 1-1");
	assetManager.MakeAsset(*textureType, "parent 1/z/b");
	assetManager.MakeAsset(*textureType, "parent 1/z/a");
	assetManager.MakeAsset(*textureType, "at the root");
	assetManager.MakeAsset(*shaderGraphType, "test graph");
	assetManager.MakeAsset(*voxelPaletteType, "city_palette");
	assetItemManager.Update();

	setWindowTitle(tr("Asset Browser"));

	QWidget* mainWidget = new QWidget();
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
	mainWidget->setLayout(layout);

	QPushButton* importButton = new QPushButton("Import");
	layout->addWidget(importButton);
	connect(importButton, &QPushButton::clicked, this, &AssetBrowser::OnImportClicked);

	QSplitter* splitter = new QSplitter(Qt::Vertical);
	layout->addWidget(splitter);

	assetTreeModel.root = assetItemManager.root.get();
	assetTreeModel.foldersOnly = false;
	assetTreeModel.recursive = true;
	assetTreeView = new QTreeView();
	splitter->addWidget(assetTreeView);
	assetTreeView->setModel(&assetTreeModel);
	assetTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	assetTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(assetTreeView, &QAbstractItemView::clicked, this, &AssetBrowser::OnItemClicked);
	connect(assetTreeView, &QAbstractItemView::activated, this, &AssetBrowser::OnItemActivated);

	quickAssetEdit = new QPropertyTree();
	splitter->addWidget(quickAssetEdit);
	quickAssetEdit->setUndoEnabled(true, false);
	quickAssetEdit->setMinimumWidth(320);
	quickAssetEdit->attach(yasli::Serializer(quickeditAdapter));

	setWidget(mainWidget);
}



void AssetBrowser::OnItemActivated(const QModelIndex& index)
{
	AssetItem* item = assetTreeModel.GetItem(index);
	if (item->asset != nullptr)
		mainFrame.OpenAssetItem(item);
}



void AssetBrowser::OnItemClicked(const QModelIndex& index)
{
	AssetItem* item = assetTreeModel.GetItem(index);
	if (item->asset != nullptr)
		QuickEditItem(item);
}



void AssetBrowser::OnImportClicked()
{
	QFileDialog dialog(this);
	dialog.setAcceptMode(QFileDialog::AcceptOpen);
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setWindowTitle(QString("Import"));

	QStringList filters;
	filters.append(QString("Bitmap (*.bmp)"));
	filters.append(QString("Targa (*.tag)"));
	filters.append(QString("PNG (*.png)"));
	filters.append(QString("JPeg (*.jpg)"));
	filters.append(QString("OBJ Meshes (*.obj)"));
	filters.append(QString("FBX Models (*.fbx)"));
	filters.append(QString("Magica Voxel (*.vox)"));
	dialog.setNameFilters(filters);

	if (dialog.exec() && !dialog.selectedFiles().isEmpty())
	{
	}
}



void AssetBrowser::QuickEditItem(AssetItem* item)
{
	if (!quickAssetEdit)
		return;
	if (item)
		quickeditAdapter.asset = item->asset;
	else
		quickeditAdapter.asset = nullptr;
	quickAssetEdit->revert();
}



AssetManager& AssetBrowser::GetAssetManager()
{
	return mainFrame.GetEditor().GetAssetManager();
}



AssetItemManager& AssetBrowser::GetAssetItemManager()
{
	return mainFrame.GetEditor().GetAssetItemManager();
}



AssetEditor::AssetEditor(MainFrame* parent, AssetItem* _assetItem)
	: QMainWindow(parent)
	, mainFrame(*parent)
	, assetItem(_assetItem)
{
}



void AssetEditor::closeEvent(QCloseEvent* event)
{
	mainFrame.CloseAssetItem(assetItem);
	QWidget::closeEvent(event);
}



MainFrame::MainFrame(VIEditor& _editor)
	: editor(_editor)
{
	setWindowTitle(tr("Volumetric Illusions Editor"));
	setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

	assetBrowser = new AssetBrowser(*this);
	addDockWidget(Qt::LeftDockWidgetArea, assetBrowser);

	mdiArea = new QMdiArea();
	setCentralWidget(mdiArea);
	mdiArea->setViewMode(QMdiArea::TabbedView);
		
	assetCreators[GetAssetManager().FindTypeTraits("Texture").get()] = &TextureEditor::Create;
	assetCreators[GetAssetManager().FindTypeTraits("ShaderGraph").get()] = &ShaderGraphEditor::Create;
	assetCreators[GetAssetManager().FindTypeTraits("VoxelPalette").get()] = &VoxelPaletteEditor::Create;
}



void MainFrame::OpenAssetItem(AssetItem* assetItem)
{
	QWidget* assetEditor = nullptr;
	auto it = assetEditors.find(assetItem->asset->GetAssetId());
	if (it == assetEditors.end())
	{
		auto creatorIt = assetCreators.find(assetItem->asset->GetAssetTraits());
		if (creatorIt != assetCreators.end())
		{
			AssetEditorCreatorFn creator = creatorIt->second;
			assetEditor = (*creator)(this, assetItem);
			QMdiSubWindow* subWindow = mdiArea->addSubWindow(assetEditor);
			subWindow->setWindowTitle(assetItem->GetFullName() + " (" + assetItem->GetFullTypeName() + ")");
		}
		else
		{
			InspectorWidget* inspector = new InspectorWidget(this, assetItem);
			assetEditor = inspector;
			addDockWidget(Qt::RightDockWidgetArea, inspector);
		}
		assetEditors[assetItem->asset->GetAssetId()] = assetEditor;
		assetEditor->setWindowTitle(assetItem->GetFullName() + " (" + assetItem->GetFullTypeName() + ")");
	}
	else
		assetEditor = it->second;
	assetEditor->setFocus();
	assetEditor->show();
}



void MainFrame::CloseAssetItem(AssetItem* assetItem)
{
	auto it = assetEditors.find(assetItem->asset->GetAssetId());
	if (it == assetEditors.end())
		return;
	it->second->close();
	assetEditors.erase(it);
}



AssetManager& MainFrame::GetAssetManager()
{
	return editor.GetAssetManager();
}


////


#if 0

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>

#include <QtWidgets/QApplication>

#include <nodes/DataModelRegistry>

#include "../ext/nodeeditor/examples/example2/TextSourceDataModel.hpp"
#include "../ext/nodeeditor/examples/example2/TextDisplayDataModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowView;
using QtNodes::FlowScene;

static std::shared_ptr<DataModelRegistry> registerDataModels()
{
	auto ret = std::make_shared<DataModelRegistry>();
	ret->registerModel<TextSourceDataModel>();
	ret->registerModel<TextDisplayDataModel>();
	return ret;
}

ShaderGraphEditor::ShaderGraphEditor(MainFrame* parent, AssetItem* assetItem)
	: AssetEditor(parent, assetItem)
{
	FlowScene* scene = new FlowScene(registerDataModels());
	FlowView* view = new FlowView(scene);
	setCentralWidget(view);
}

#endif

#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>
#include <nodes/ConnectionStyle>

#include <QtWidgets/QApplication>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QMenuBar>

#include <nodes/DataModelRegistry>

#include "../ext/nodeeditor/examples/calculator/NumberSourceDataModel.hpp"
#include "../ext/nodeeditor/examples/calculator/NumberDisplayDataModel.hpp"
#include "../ext/nodeeditor/examples/calculator/AdditionModel.hpp"
#include "../ext/nodeeditor/examples/calculator/SubtractionModel.hpp"
#include "../ext/nodeeditor/examples/calculator/MultiplicationModel.hpp"
#include "../ext/nodeeditor/examples/calculator/DivisionModel.hpp"
#include "../ext/nodeeditor/examples/calculator/ModuloModel.hpp"
#include "../ext/nodeeditor/examples/calculator/DecimalToIntegerModel.hpp"
#include "../ext/nodeeditor/examples/calculator/IntegerToDecimalModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowScene;
using QtNodes::FlowView;
using QtNodes::ConnectionStyle;


class ConnectorData : public NodeData
{
public:
	ConnectorData(const char* _name, bool _permutation = false)
		: name(_name)
		, permutation(_permutation) {}

	NodeDataType type() const override
	{
		return NodeDataType{ permutation ? "permutation" : "data", name };
	}

	const char* name;
	const bool permutation;
};



class ShaderNode : public NodeDataModel
{
public:
	ShaderNode(const char* _nodeName)
		: edit(nullptr)
		, widget(nullptr)
		, nodeName(_nodeName)
	{
	}

	QString caption() const final
	{
		return QString(nodeName.c_str());
	}

	bool captionVisible() const final
	{
		return true;
	}

	QString name() const final
	{
		return QString(nodeName.c_str());
	}

	virtual void serialize(yasli::Archive& ar)
	{
		assert(false);	// override serialize and don't inherit
	}

	QWidget* embeddedWidget() final
	{
		return widget;
	}

	const char* GetNodeName() const { return nodeName.c_str(); }

protected:
	void AddEditWidget()
	{
		if (widget != nullptr)
			return;
		/*
		// widget = new QLineEdit();
		widget = new QWidget();

		QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
		widget->setLayout(layout);

		layout->addWidget(new QLineEdit());

		edit = new QPropertyTree();
		edit->setUndoEnabled(true, false);
		edit->attach(yasli::Serializer(*this));
		edit->setMinimumWidth(280);
		layout->addWidget(edit);
		*/

		widget = new QLineEdit();
	}

	void AddInput(const char* name, bool permutation = false)
	{
		inputNames.emplace_back(name);
		inputIsPermutation.emplace_back(permutation ? 1 : 0);
		inputs.emplace_back();
	};

	void AddOutput(const char* name, bool permutation = false)
	{
		outputs.emplace_back(std::make_shared<ConnectorData>(name, permutation));
	};

	unsigned int nPorts(PortType portType) const final
	{
		switch (portType)
		{
		case PortType::In: return inputs.size();
		case PortType::Out: return outputs.size();
		}
		return 0;
	}

	NodeDataType dataType(PortType portType, PortIndex portIndex) const final
	{
		switch (portType)
		{
		case PortType::In: return ConnectorData(inputNames[portIndex].c_str(), !!inputIsPermutation[portIndex]).type();
		case PortType::Out: return outputs[portIndex]->type();
		}
		return NodeDataType();
	}

	std::shared_ptr<NodeData> outData(PortIndex port) final
	{
		return outputs[port];
	}

	void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) final
	{
		inputs[portIndex] = std::dynamic_pointer_cast<ConnectorData>(data);
	}

private:
	QWidget* widget;
	QPropertyTree* edit;
	std::vector<std::shared_ptr<ConnectorData>> outputs;
	std::vector<std::weak_ptr<ConnectorData>> inputs;
	std::vector<std::string> inputNames;
	std::vector<char> inputIsPermutation;
	std::string nodeName;
};


class InputNode : public ShaderNode
{
public:
	InputNode()
		: ShaderNode("Input")
	{
		AddEditWidget();
		AddInput("Default");
		AddInput("Tweakable", true);
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<InputNode>();
	}

	void serialize(yasli::Archive& ar) override
	{
		ar(inputName, "name", "");
	}

private:
	std::string inputName;
};



class OutputNode : public ShaderNode
{
public:
	OutputNode()
		: ShaderNode("Output")
	{
		AddEditWidget();
		AddInput("test");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<OutputNode>();
	}

	void serialize(yasli::Archive& ar) override
	{
		ar(outputName, "name", "");
	}

private:
	std::string outputName;
};



class NodeOperator : public ShaderNode
{
public:
	NodeOperator() = delete;

	NodeOperator(const char* _opName)
		: ShaderNode(_opName)
	{
		AddInput("In0");
		AddInput("In1");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeOperator>(GetNodeName());
	}
};



class AddNode : public NodeOperator
{
public:
	AddNode() : NodeOperator("Add") {}
};

class SubtractNode : public NodeOperator
{
public:
	SubtractNode() : NodeOperator("Subtract") {}
};

class MultiplyNode : public NodeOperator
{
public:
	MultiplyNode() : NodeOperator("Multiply") {}
};

class DivideNode : public NodeOperator
{
public:
	DivideNode() : NodeOperator("Divide") {}
};

class NodeLerp : public ShaderNode
{
public:
	NodeLerp()
		: ShaderNode("Lerp")
	{
		AddInput("Min");
		AddInput("Max");
		AddInput("X");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeLerp>();
	}
};

class NodeClamp : public ShaderNode
{
public:
	NodeClamp()
		: ShaderNode("Lerp")
	{
		AddInput("Min");
		AddInput("Max");
		AddInput("X");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeClamp>();
	}
};

class NodeSaturate : public ShaderNode
{
public:
	NodeSaturate()
		: ShaderNode("Saturate")
	{
		AddInput("In");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeSaturate>();
	}
};

class NodeNormalize : public ShaderNode
{
public:
	NodeNormalize()
		: ShaderNode("Normalize")
	{
		AddInput("In");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeNormalize>();
	}
};

class NodeNegate : public ShaderNode
{
public:
	NodeNegate()
		: ShaderNode("Negate")
	{
		AddInput("In");
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeNegate>();
	}
};

class NodeParameter : public ShaderNode
{
public:
	NodeParameter()
		: ShaderNode("Parameter")
	{
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeParameter>();
	}
};

class NodeMix : public ShaderNode
{
public:
	NodeMix()
		: ShaderNode("Mix")
	{
		AddInput("In");
		AddInput("X");
		AddInput("Y");
		AddInput("Z");
		AddInput("W");

		AddOutput("Out");
		AddOutput("X");
		AddOutput("Y");
		AddOutput("Z");
		AddOutput("W");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeMix>();
	}
};


class NodeSelect : public ShaderNode
{
public:
	NodeSelect()
		: ShaderNode("Select")
	{
		AddInput("True");
		AddInput("False");
		AddInput("Cond", true);
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeSelect>();
	}
};


class NodeTexture : public ShaderNode
{
public:
	NodeTexture()
		: ShaderNode("Texture")
	{
		AddEditWidget();
		AddInput("Coord");
		AddInput("Slice");
		AddOutput("Out");
		AddOutput("Is Set", true);
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeTexture>();
	}

	void serialize(yasli::Archive& ar) override
	{
		ar(paramName, "name", "");
	}

private:
	std::string paramName;
};



class NodeVertexInput : public ShaderNode
{
public:
	NodeVertexInput()
		: ShaderNode("Vertex Input")
	{
		AddOutput("Out");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeVertexInput>();
	}

	void serialize(yasli::Archive& ar) override
	{
	}

private:
};



class NodeNoise : public ShaderNode
{
public:
	NodeNoise()
		: ShaderNode("Noise")
	{
		AddEditWidget();
		AddInput("Coord");
		AddOutput("Potential");
		AddOutput("Gradient");
		AddOutput("Curl");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeNoise>();
	}

	void serialize(yasli::Archive& ar) override
	{
	}

private:
};


class MaterialNode : public ShaderNode
{
public:
	MaterialNode()
		: ShaderNode("Material")
	{
		AddEditWidget();
		AddInput("Albedo");
		AddInput("Glossiness");
		AddInput("Metalicity");
		AddInput("Emission");
		AddInput("Normal");
		AddInput("IOR");
		AddInput("Opacity");
		AddInput("Position Offset");
		AddInput("Alpha Test", true);
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<MaterialNode>();
	}

	void serialize(yasli::Archive& ar) override
	{
	}

private:
};



class NodeFunction : public ShaderNode
{
public:
	NodeFunction(const char* _functionName)
		: ShaderNode(_functionName)
		, functionName(_functionName)
	{
		AddInput("Tangent");
		AddOutput("World");
	}

public:
	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<NodeFunction>(functionName);
	}

private:
	const char* functionName;
};



static std::shared_ptr<DataModelRegistry> registerDataModels()
{
	static bool ready = false;

	static auto ret = std::make_shared<DataModelRegistry>();

	if (ready)
		return ret;

	ret->registerModel<InputNode>("Parameters");
	ret->registerModel<OutputNode>("Parameters");
	ret->registerModel<NodeTexture>("Parameters");
	ret->registerModel<NodeVertexInput>("Parameters");
	ret->registerModel<NodeParameter>("Parameters");

	ret->registerModel<AddNode>("Arithmatics");
	ret->registerModel<SubtractNode>("Arithmatics");
	ret->registerModel<MultiplyNode>("Arithmatics");
	ret->registerModel<DivideNode>("Arithmatics");
	ret->registerModel<NodeClamp>("Arithmatics");
	ret->registerModel<NodeLerp>("Arithmatics");
	ret->registerModel<NodeSaturate>("Arithmatics");
	ret->registerModel<NodeNegate>("Arithmatics");
	ret->registerModel<NodeNormalize>("Arithmatics");
	ret->registerModel<NodeNoise>("Arithmatics");
	ret->registerModel<NodeMix>("Arithmatics");

	ret->registerModel<NodeSelect>("Permutation");
	
	ret->registerModel<MaterialNode>("Graph");

	ready = true;

	return ret;
}



static std::shared_ptr<DataModelRegistry> getFlowModels()
{
	static bool ready = false;
	static auto ret = std::make_shared<DataModelRegistry>();

	if (ready)
		return ret;

	ready = true;
	return ret;
}

class FlowData : public NodeData
{
public:
	FlowData(const char* _name)
		: name(_name) {}

	NodeDataType type() const override
	{
		return NodeDataType{ "data", name };
	}

	const char* name;
};

class FlowNode : public NodeDataModel
{
public:
	FlowNode(const char* _nodeName, const char* _captionName)
		: nodeName(_nodeName)
		, captionName(_captionName)
	{
	}

	QString caption() const final
	{
		return QString(captionName.c_str());
	}

	bool captionVisible() const final
	{
		return true;
	}

	QString name() const final
	{
		return QString(nodeName.c_str());
	}

	QWidget* embeddedWidget() final
	{
		return nullptr;
	}

	virtual void serialize(yasli::Archive& ar)
	{
		assert(false);	// override serialize and don't inherit
	}

	const char* GetNodeName() const { return nodeName.c_str(); }

protected:
	void AddInput(const char* name)
	{
		inputNames.emplace_back(name);
		inputs.emplace_back();
	};

	void AddOutput(const char* name)
	{
		outputs.emplace_back(std::make_shared<FlowData>(name));
	};

	unsigned int nPorts(PortType portType) const final
	{
		switch (portType)
		{
		case PortType::In: return inputs.size();
		case PortType::Out: return outputs.size();
		}
		return 0;
	}

	NodeDataType dataType(PortType portType, PortIndex portIndex) const final
	{
		switch (portType)
		{
		case PortType::In: return FlowData(inputNames[portIndex].c_str()).type();
		case PortType::Out: return outputs[portIndex]->type();
		}
		return NodeDataType();
	}

	std::shared_ptr<NodeData> outData(PortIndex port) final
	{
		return outputs[port];
	}

	void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) final
	{
		inputs[portIndex] = std::dynamic_pointer_cast<FlowData>(data);
	}

private:
	QWidget* widget;
	QPropertyTree* edit;
	std::vector<std::shared_ptr<FlowData>> outputs;
	std::vector<std::weak_ptr<FlowData>> inputs;
	std::vector<std::string> inputNames;
	std::string nodeName;
	std::string captionName;
};

#define FLOW_NODE_REGISTER(CATEGORY, NAME) \
	static struct Flow##CATEGHORY##NAME { \
		Flow##CATEGHORY##NAME() { getFlowModels()->registerModel<NAME>(#CATEGORY); } \
	} flow##CATEGHORY##NAME;

class EntityPosition : public FlowNode
{
public:
	EntityPosition()
		: FlowNode("Position", "Entity" " : " "Position")
	{
		AddInput("EntityId");
		AddOutput("Position");
	}

	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<EntityPosition>();
	}
};
FLOW_NODE_REGISTER(Entity, EntityPosition)

class EntityThisId : public FlowNode
{
public:
	EntityThisId()
		: FlowNode("ThisId", "Entity" " : " "ThisId")
	{
		AddOutput("EntityId");
	}

	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<EntityThisId>();
	}
};
FLOW_NODE_REGISTER(Entity, EntityThisId)

class GameStart : public FlowNode
{
public:
	GameStart()
		: FlowNode("Start", "Game" " : " "Start")
	{
		AddOutput("Level Start");
	}

	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<GameStart>();
	}
};
FLOW_NODE_REGISTER(Game, GameStart)

class GamePlayerId : public FlowNode
{
public:
	GamePlayerId()
		: FlowNode("Start", "Game" " : " "PlayerId")
	{
		AddOutput("PlayerId");
	}

	std::unique_ptr<NodeDataModel> clone() const override
	{
		return std::make_unique<GamePlayerId>();
	}
};
FLOW_NODE_REGISTER(Game, GamePlayerId)




static void nodeSetStyle()
{
	ConnectionStyle::setConnectionStyle(
		R"(
			  {
				"ConnectionStyle": {
				  "ConstructionColor": "gray",
				  "NormalColor": "black",
				  "SelectedColor": "gray",
				  "SelectedHaloColor": "deepskyblue",
				  "HoveredColor": "deepskyblue",

				  "LineWidth": 3.0,
				  "ConstructionLineWidth": 2.0,
				  "PointDiameter": 10.0,

				  "UseDataDefinedColors": true
				}
			  }
			  )");
}


ShaderGraphEditor::ShaderGraphEditor(MainFrame* parent, AssetItem* assetItem)
	: AssetEditor(parent, assetItem)
{
	nodeSetStyle();

	auto menuBar = new QMenuBar();
	auto saveAction = menuBar->addAction("Save..");
	auto loadAction = menuBar->addAction("Load..");

//	FlowScene* scene = new FlowScene(registerDataModels());
//	scene->registry().registerModel<NodeFunction>("Functions", std::make_unique<NodeFunction>("TangentNormal"));
//	scene->registry().registerModel<NodeFunction>("Functions", std::make_unique<NodeFunction>("ExpandNormal"));
//	scene->registry().registerModel<NodeFunction>("Functions", std::make_unique<NodeFunction>("NormalBlend"));

	FlowScene* scene = new FlowScene(getFlowModels());

	InspectorWidget* inspector = new InspectorWidget(parent, assetItem);
	addDockWidget(Qt::RightDockWidgetArea, inspector);

	FlowView* view = new FlowView(scene);
	setCentralWidget(view);

	QObject::connect(saveAction, &QAction::triggered,
		scene, &FlowScene::save);

	QObject::connect(loadAction, &QAction::triggered,
		scene, &FlowScene::load);
}



using namespace viqt;



Renderer::Renderer()
{
	renderDevice = RenderDevice::Create(RenderDeviceDesc());
	shaderManager = std::make_shared<ShaderManager>(renderDevice);
	commandBuffer = std::make_shared<CommandBuffer>(renderDevice);
}



class WorldRenderWidget : public Dx11Widget
{
public:
	WorldRenderWidget(QWidget* parent, Renderer& _renderer)
		: Dx11Widget(parent, _renderer.GetRenderDevice())
		, renderer(_renderer) {}

	void Render() override
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		CommandBuffer& commandBuffer = *renderer.GetCommandBuffer().get();

		{
			VIDF_GPU_EVENT(renderDevice, Clear);
			const float clearColor[] = { 0, 0, 0.2f, 0 };
			commandBuffer.GetContext()->ClearRenderTargetView(outputRTV, clearColor);
		}

		{
			VIDF_GPU_EVENT(renderDevice, Display);

			commandBuffer.BeginRenderPass(displayPass);
			commandBuffer.EndRenderPass();
		}

		GetSwapChain()->Present(false);
		update();
	}

	void Resize(int width, int heigth) override
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = heigth;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		RenderPassDesc displayPassDesc;
		displayPassDesc.viewport = viewport;
		displayPassDesc.rtvs[0] = GetSwapChain()->GetBackBufferRTV();
		displayPass = RenderPass::Create(renderDevice, displayPassDesc);

		if (!outputRTV)
		{
			outputRTV = GetSwapChain()->GetBackBufferRTV();
			CompileShaders();
			PreparePipeline();
		}
	}

private:
	void CompileShaders()
	{
	}

	void PreparePipeline()
	{
	}

private:
	Renderer&              renderer;
	PD3D11RenderTargetView outputRTV;
	RenderPassPtr          displayPass;
};



void VoxelPaletteAsset::serialize(yasli::Archive& ar)
{
	Asset::serialize(ar);
	for (uint i = 0; i < materials.size(); ++i)
		ar(materials[i], "materials", "Materials");
	ar(entries, "entries", "Entries");
}



VoxelPaletteEditor::VoxelPaletteEditor(MainFrame* parent, AssetItem* assetItem)
	: AssetEditor(parent, assetItem)
{
	WorldRenderWidget* view = new WorldRenderWidget(this, parent->GetEditor().GetRenderer());
	setCentralWidget(view);

	InspectorWidget* inspector = new InspectorWidget(parent, assetItem);
	addDockWidget(Qt::RightDockWidgetArea, inspector);
}



////



int AssetBrowserTest(int argc, char** argv)
{
	VIEditor app{ argc, argv };

	MainFrame mainFrame{ app };
	mainFrame.showMaximized();

	return app.exec();
}
