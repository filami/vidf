#pragma once

#include <unordered_map>
#include <random>
#include <ctime>
#include <QMainWindow>
#include <QTreeWidget>
#include <QListView>
#include <QTableView>
#include <QFileIconProvider>
#include <QKeyEvent>
#include <QLabel>
#include <QDockWidget>
#include <vidf/pch.h>
#include "QPropertyTree/QPropertyTree.h"

#include <QListWidget>

using namespace vidf;


typedef uint64 AssetId;


class Asset;
struct AssetTraits;
typedef std::shared_ptr<Asset> AssetPtr;


struct AssetRef
{
	AssetId      id = -1;
	std::string  name;
	AssetPtr     asset;
	AssetTraits* traits;
};



struct AssetTraits
{
	virtual ~AssetTraits() {}
	virtual Asset*       Create(AssetRef& assetRef) = 0;
	virtual const char*  GetTypeName() const = 0;
	virtual AssetTraits* GetParent() const { return nullptr; }
	virtual bool         IsAbstract() const { return false; }
};



class Asset
{
public:
	Asset(AssetRef& _assetRef)
		: assetRef(_assetRef) {}
	virtual ~Asset() {}

	virtual void serialize(yasli::Archive& ar) { }
	virtual void SerializeQuickEdit(yasli::Archive& ar) { serialize(ar); }

	const AssetRef& assetRef;
};



class AssetManager
{
public:
	typedef std::unordered_map<AssetId, AssetRef> AssetMap;

public:
	AssetManager()
		: random(std::time(0)) {}

	AssetPtr MakeAsset(AssetTraits* traits, const char* name);
	AssetMap::const_iterator begin() const { return assets.begin(); }
	AssetMap::const_iterator end() const { return assets.end(); }

private:
	AssetId MakeUniqueId();

private:
	AssetMap        assets;
	std::mt19937_64 random;
};



//////////////////////////////////////////////////////////////////////////


struct AssetItem;
typedef std::shared_ptr<AssetItem> AssetItemPtr;
typedef std::weak_ptr<AssetItem> AssetItemRef;



struct AssetItem
{
	std::vector<AssetItemPtr> children;
	AssetItemRef parent;
	std::string  name;
	AssetRef     assetRef;
	int          row = 0;

	QString GetFullName() const;
	QString GetFullTypeName() const;
};



struct AssetItemManager
{
	AssetItemManager(AssetManager* _assetManager)
		: assetManager(_assetManager)
		, root(std::make_shared<AssetItem>())
		, virtualRoot(std::make_shared<AssetItem>())
	{
		root->name = "root";
		root->parent = virtualRoot;
		virtualRoot->children.push_back(root);
	}

	AssetManager* assetManager;
	AssetItemPtr  virtualRoot;
	AssetItemPtr  root;

	void Update();
	void InsertItem(AssetItemPtr parent, const std::string& name, const AssetRef& ref);
	void SortItems(AssetItemPtr item, bool recursive);
};



class AssetBrowserModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	AssetBrowserModel(AssetItem* root);

	QVariant data(const QModelIndex& index, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	AssetItem* root;
	bool foldersOnly = false;
	bool recursive = true;

// private:
	AssetItem* GetItem(const QModelIndex& index) const;

private:
	QFileIconProvider iconsProvider;
};



class MainFrame;



class AssetBrowser : public QDockWidget
{
	Q_OBJECT
private:
	struct QuickSerializerAdapter
	{
		void serialize(yasli::Archive& ar)
		{
			if (asset)
				asset->SerializeQuickEdit(ar);
		}
		AssetPtr asset;
	};

public:
	AssetBrowser(MainFrame& _mainFrame);

private slots:
	void OnFolderSelected(const QModelIndex& index);
	void OnItemActivated(const QModelIndex& index);
	void OnItemClicked(const QModelIndex& index);

private:
	void keyPressEvent(QKeyEvent* event) override;
	void SetSubItemRoot(AssetItem* newRoot);
	void QuickEditItem(AssetItem* item);

private:
	MainFrame&        mainFrame;

	AssetManager      assetManager;
	AssetItemManager  assetItemManager;

	AssetBrowserModel assetTreeModel;
	QTreeView*        assetTreeView;

	AssetBrowserModel assetTreeModel2;
	QTreeView*        assetTreeView2;

	QLabel*           pathWidget;

	QuickSerializerAdapter quickeditAdapter;
	QPropertyTree*    quickAssetEdit = nullptr;

	AssetTraits* testTraits;
};



class AssetEditor;



class MainFrame : public QMainWindow
{
	Q_OBJECT
public:
	MainFrame();

	void OpenAssetItem(AssetItem* assetItem);
	void CloseAssetItem(AssetItem* assetItem);

private:
	void AddDockWindow(QDockWidget* dockWindow);

private:
	AssetBrowser* assetBrowser;
	std::unordered_map<AssetId, AssetEditor*> assetEditors;
};



class AssetEditor : public QDockWidget
{
	Q_OBJECT
public:
	AssetEditor(MainFrame* parent, AssetItem* _assetItem);
	virtual ~AssetEditor() {}
	AssetItem* assetItem;

private:
	void AssetEditor::closeEvent(QCloseEvent* event) override;

private:
	MainFrame& mainFrame;
};



class SimpleAssetEditor : public AssetEditor
{
	Q_OBJECT
public:
	SimpleAssetEditor(MainFrame* parent, AssetItem* _assetItem)
		: AssetEditor(parent, _assetItem)
	{
		assetEdit = new QPropertyTree();
		assetEdit->setUndoEnabled(true, false);
		assetEdit->attach(yasli::Serializer(*this));
		setWidget(assetEdit);
	}

	void serialize(yasli::Archive& ar)
	{
		assetItem->assetRef.asset->SerializeQuickEdit(ar);
	}

	QPropertyTree* assetEdit;
};
