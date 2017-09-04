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
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QBoxLayout>
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
	void OnItemActivated(const QModelIndex& index);
	void OnItemClicked(const QModelIndex& index);

private:
	void              QuickEditItem(AssetItem* item);
	AssetManager&     GetAssetManager();
	AssetItemManager& GetAssetItemManager();

private:
	MainFrame&        mainFrame;

	AssetBrowserModel assetTreeModel;
	QTreeView*        assetTreeView;
	QuickSerializerAdapter quickeditAdapter;
	QPropertyTree*    quickAssetEdit = nullptr;
};



class AssetEditor;



class MainFrame : public QMainWindow
{
	Q_OBJECT
private:
	typedef AssetEditor*(*AssetEditorCreatorFn)(MainFrame*, AssetItem*);

public:
	MainFrame(VIEditor& _editor);

	void      OpenAssetItem(AssetItem* assetItem);
	void      CloseAssetItem(AssetItem* assetItem);
	VIEditor& GetEditor() { return editor; }

private:
	AssetManager& GetAssetManager();

private:
	VIEditor&        editor;
	AssetBrowser*    assetBrowser;
	QMdiArea*        mdiArea;
	std::unordered_map<AssetId, QWidget*> assetEditors;
	std::unordered_map<const AssetTraits*, AssetEditorCreatorFn> assetCreators;
};



class InspectorWidget : public QDockWidget
{
	Q_OBJECT
public:
	InspectorWidget(MainFrame* parent, AssetItem* _assetItem)
		: QDockWidget(parent)
		, assetItem(_assetItem)
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

private:
	AssetItem*     assetItem;
	QPropertyTree* assetEdit;
};



class AssetEditor : public QMainWindow
{
	Q_OBJECT
public:
	AssetEditor(MainFrame* parent, AssetItem* _assetItem);
	virtual ~AssetEditor() { }

private:
	void AssetEditor::closeEvent(QCloseEvent* event) override;

private:
	MainFrame& mainFrame;
	AssetItem* assetItem;
};



class TextureEditor : public AssetEditor
{
public:
	TextureEditor(MainFrame* parent, AssetItem* assetItem)
		: AssetEditor(parent, assetItem)
	{
		QWidget* textureViewer = new QWidget();
		setCentralWidget(textureViewer);

		InspectorWidget* inspector = new InspectorWidget(parent, assetItem);
		addDockWidget(Qt::RightDockWidgetArea, inspector);
	}

	static AssetEditor* Create(MainFrame* parent, AssetItem* assetItem) { return new TextureEditor(parent, assetItem); }

private:
};
