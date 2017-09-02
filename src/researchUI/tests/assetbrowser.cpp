#include "pch.h"
#include "assetbrowser.h"
#include <QFileSystemModel>
#include <QSplitter>



class Texture : public Asset
{
public:
	Texture(AssetRef& _assetRef)
		: Asset(_assetRef) {}

	virtual void serialize(yasli::Archive& ar) { ar(test, "test", "Test"); }

	int test = 0;
};



struct TextureType : public AssetTraits
{
	virtual Asset*       Create(AssetRef& assetRef) { return new Texture(assetRef); }
	virtual const char*  GetTypeName() const { return "Texture"; }
};



//////////////////////////////////////////////////////////////////////////



QString AssetItem::GetFullName() const
{
	auto parentPtr = parent.lock();
	if (!parentPtr)
		return QString();
	return parentPtr->GetFullName() + QString("/") + QString(name.c_str());
}



QString AssetItem::GetFullTypeName() const
{
	return QString(assetRef.traits->GetFullTypeName().c_str());
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
			if (!item->assetRef.asset)
				return iconsProvider.icon(QFileIconProvider::Folder);
			else
				return iconsProvider.icon(QFileIconProvider::File);
		}
	case Qt::DisplayRole:
		if (item->assetRef.asset == nullptr)
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
		case 2: return item->assetRef.id;
		default: return QString();
		}
	default:
		return QVariant();
	}
}



Qt::ItemFlags AssetBrowserModel::flags(const QModelIndex& index) const
{
	return 0; // ??
}



QVariant AssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal && role != Qt::DisplayRole)
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
	if (foldersOnly && childItem->assetRef.asset != nullptr)
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
			if (it->assetRef.asset == nullptr)
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
		const AssetRef& ref = it.second;
		InsertItem(root, ref.name, it.second);
	}
	const bool recursive = true;
	SortItems(root, recursive);
}



void AssetItemManager::InsertItem(AssetItemPtr parent, const std::string& name, const AssetRef& ref)
{
	assert(name[0] != '/'); // ?? no name?
	size_t offset = name.find('/');
	if (offset == size_t(-1))
	{
		AssetItemPtr subItem = std::make_shared<AssetItem>();
		subItem->parent = parent;
		subItem->name = name;
		subItem->assetRef = ref;
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
		assert(subItem->assetRef.asset == nullptr);		// sub item has to be a folder and not anohter real asset
		InsertItem(subItem, restName, ref);
	}
}



void AssetItemManager::SortItems(AssetItemPtr item, bool recursive)
{
	std::sort(
		item->children.begin(), item->children.end(),
		[](AssetItemPtr left, AssetItemPtr right)
	{
		if (left->assetRef.asset != nullptr && right->assetRef.asset == nullptr)
			return false;
		if (left->assetRef.asset == nullptr && right->assetRef.asset != nullptr)
			return true;
		return left->name < right->name;
	});
	for (uint i = 0; i < item->children.size(); ++i)
		item->children[i]->row = i;
	if (recursive)
	{
		for (auto child : item->children)
			SortItems(child, true);
	}
}



AssetBrowser::AssetBrowser(MainFrame& _mainFrame)
	: QDockWidget(&_mainFrame)
	, mainFrame(_mainFrame)
{
	auto& assetManager = GetAssetManager();
	auto& assetItemManager = GetAssetItemManager();
	AssetTraitsPtr textureType = assetManager.FindTypeTraits("Texture");

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

	pathWidget = new QLabel();
	pathWidget->setText(QString("test"));
	layout->addWidget(pathWidget);

	{
		QBoxLayout* subLayout = new QBoxLayout(QBoxLayout::LeftToRight);
		layout->addLayout(subLayout);

		QSplitter* splitter = new QSplitter(Qt::Horizontal);
		subLayout->addWidget(splitter);

		assetTreeModel.root = assetItemManager.root.get();
		assetTreeModel.foldersOnly = true;
		assetTreeModel.recursive = true;
		assetTreeView = new QTreeView();
		splitter->addWidget(assetTreeView);
		assetTreeView->setModel(&assetTreeModel);
		assetTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
		assetTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
		assetTreeView->setHeaderHidden(true);
		connect(assetTreeView, &QAbstractItemView::clicked, this, &AssetBrowser::OnFolderSelected);

		assetTreeModel2.foldersOnly = false;
		assetTreeModel2.recursive = false;
		assetTreeView2 = new QTreeView();
		splitter->addWidget(assetTreeView2);
		assetTreeView2->setModel(&assetTreeModel2);
		assetTreeView2->setSelectionMode(QAbstractItemView::SingleSelection);
		assetTreeView2->setSelectionBehavior(QAbstractItemView::SelectRows);
		SetSubItemRoot(assetItemManager.root.get());
		connect(assetTreeView2, &QAbstractItemView::clicked, this, &AssetBrowser::OnItemClicked);
		connect(assetTreeView2, &QAbstractItemView::activated, this, &AssetBrowser::OnItemActivated);

		quickAssetEdit = new QPropertyTree();
		subLayout->addWidget(quickAssetEdit);
		quickAssetEdit->setUndoEnabled(true, false);
		quickAssetEdit->setMaximumWidth(380);
		quickAssetEdit->attach(yasli::Serializer(quickeditAdapter));
	}

	setWidget(mainWidget);
}



void AssetBrowser::OnFolderSelected(const QModelIndex& index)
{
	SetSubItemRoot(assetTreeModel.GetItem(index));
}



void AssetBrowser::OnItemActivated(const QModelIndex& index)
{
	AssetItem* item = assetTreeModel.GetItem(index);
	if (item->assetRef.asset == nullptr)
		SetSubItemRoot(item);
	else
		mainFrame.OpenAssetItem(item);
}



void AssetBrowser::OnItemClicked(const QModelIndex& index)
{
	AssetItem* item = assetTreeModel.GetItem(index);
	if (item->assetRef.asset != nullptr)
		QuickEditItem(item);
}



void AssetBrowser::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Backspace)
	{
		if (assetTreeModel2.root != GetAssetItemManager().root.get())
			SetSubItemRoot(assetTreeModel2.root->parent.lock().get());
	}
}



void AssetBrowser::SetSubItemRoot(AssetItem* newRoot)
{
	assetTreeModel2.root = newRoot;
	assetTreeView2->reset();
	pathWidget->setText(newRoot->GetFullName());
	QuickEditItem(nullptr);
}



void AssetBrowser::QuickEditItem(AssetItem* item)
{
	if (!quickAssetEdit)
		return;
	if (item)
		quickeditAdapter.asset = item->assetRef.asset;
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
	: QDockWidget(_assetItem->GetFullName() + " (" + _assetItem->GetFullTypeName() + ")", parent)
	, mainFrame(*parent)
	, assetItem(_assetItem)
{
}



void AssetEditor::closeEvent(QCloseEvent* event)
{
	mainFrame.CloseAssetItem(assetItem);
	QDockWidget::closeEvent(event);
}



MainFrame::MainFrame(VIEditor& _editor)
	: editor(_editor)
{
	setWindowTitle(tr("Volumetric Illusions Editor"));
	setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

	assetBrowser = new AssetBrowser(*this);
	AddDockWindow(assetBrowser);
}



void MainFrame::OpenAssetItem(AssetItem* assetItem)
{
	AssetEditor* assetEditor = nullptr;
	auto it = assetEditors.find(assetItem->assetRef.id);
	if (it == assetEditors.end())
	{
		assetEditor = new SimpleAssetEditor(this, assetItem);
		AddDockWindow(assetEditor);
		assetEditors[assetItem->assetRef.id] = assetEditor;
	}
	else
		assetEditor = it->second;
	assetEditor->setFocus();
	assetEditor->show();
}



void MainFrame::CloseAssetItem(AssetItem* assetItem)
{
	auto it = assetEditors.find(assetItem->assetRef.id);
	if (it == assetEditors.end())
		return;
	it->second->close();
	assetEditors.erase(it);
}



void MainFrame::AddDockWindow(QDockWidget* dockWindow)
{
	addDockWidget(Qt::LeftDockWidgetArea, dockWindow);
	if (dockWindow != assetBrowser)
		tabifyDockWidget(assetBrowser, dockWindow);
}



int AssetBrowserTest(int argc, char** argv)
{
	VIEditor app{ argc, argv };

	MainFrame mainFrame{ app };
	mainFrame.showMaximized();

	return app.exec();
}
