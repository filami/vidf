#include "pch.h"
#include "assetbrowser.h"
#include <QFileSystemModel>
#include <QSplitter>



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
	auto& assetManager = GetAssetManager();
	auto& assetItemManager = GetAssetItemManager();
	AssetTraitsPtr textureType = assetManager.FindTypeTraits("Texture");
	AssetTraitsPtr textureSettingsType = assetManager.FindTypeTraits("TextureSettings");

	assetManager.MakeAsset(*textureSettingsType, "System/UncompressedNormal");
	assetManager.MakeAsset(*textureSettingsType, "System/CompressedDiffuse");
	assetManager.MakeAsset(*textureSettingsType, "System/CompressedHDR");

	assetManager.MakeAsset(*textureType, "parent 0/child 0-0");
	assetManager.MakeAsset(*textureType, "parent 1/child 1-0");
	assetManager.MakeAsset(*textureType, "parent 1/child 1-1");
	assetManager.MakeAsset(*textureType, "parent 1/z/b");
	assetManager.MakeAsset(*textureType, "parent 1/z/a");
	assetManager.MakeAsset(*textureType, "at the root");
	assetItemManager.Update();

	setWindowTitle(tr("Asset Browser"));

	QWidget* mainWidget = new QWidget();
	QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom);
	mainWidget->setLayout(layout);

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



int AssetBrowserTest(int argc, char** argv)
{
	VIEditor app{ argc, argv };

	MainFrame mainFrame{ app };
	mainFrame.showMaximized();

	return app.exec();
}
