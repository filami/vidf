#pragma once

#include <QApplication>
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

#include "vidf/assets/assetmanager.h"

using namespace vidf;



//////////////////////////////////////////////////////////////////////////


struct AssetItem;
typedef std::shared_ptr<AssetItem> AssetItemPtr;
typedef std::weak_ptr<AssetItem> AssetItemRef;



struct AssetItem
{
	std::vector<AssetItemPtr> children;
	AssetItemRef    parent;
	std::string     name;
	AssetRef        assetRef;
	int             row = 0;

	void    SortChilden(bool recursive);
	QString GetFullName() const;
	QString GetFullTypeName() const;
	void    ChangeName(const QString& name);
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
};



class AssetBrowserModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	AssetBrowserModel();

	QVariant data(const QModelIndex& index, int role) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role);
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& index) const override;
	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;

	AssetItem* root = nullptr;
	bool foldersOnly = false;
	bool recursive = true;

// private:
	AssetItem* GetItem(const QModelIndex& index) const;

private:
	QFileIconProvider iconsProvider;
};



class MainFrame;



class VIEditor : public QApplication
{
public:
	VIEditor(int& argc, char** argv, int appFlags = ApplicationFlags)
		: QApplication(argc, argv, appFlags)
		, assetItemManager(&assetManager) {}

	AssetManager&     GetAssetManager() { return assetManager; }
	AssetItemManager& GetAssetItemManager() { return assetItemManager; }

public:
	AssetManager     assetManager;
	AssetItemManager assetItemManager;
};



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

	AssetManager&     GetAssetManager();
	AssetItemManager& GetAssetItemManager();

private:
	MainFrame&        mainFrame;

	AssetBrowserModel assetTreeModel;
	QTreeView*        assetTreeView;

	AssetBrowserModel assetTreeModel2;
	QTreeView*        assetTreeView2;

	QLabel*           pathWidget;

	QuickSerializerAdapter quickeditAdapter;
	QPropertyTree*    quickAssetEdit = nullptr;
};



class AssetEditor;



class MainFrame : public QMainWindow
{
	Q_OBJECT
public:
	MainFrame(VIEditor& _editor);

	void      OpenAssetItem(AssetItem* assetItem);
	void      CloseAssetItem(AssetItem* assetItem);
	VIEditor& GetEditor() { return editor; }

private:
	void AddDockWindow(QDockWidget* dockWindow);

private:
	VIEditor&        editor;
	AssetBrowser*    assetBrowser;
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
