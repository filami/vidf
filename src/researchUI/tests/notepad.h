#pragma once

#include "pch.h"
#include "QPropertyTree/QPropertyTree.h"

enum Position
{
	ENGINEER,
	MANAGER,
};

struct Entry
{
	std::string name = "Foo Bar";
	int age = 25;
	Position position = ENGINEER;

	void serialize(yasli::Archive& ar);
};

struct Document
{
	std::vector<Entry> entries = { Entry() };

	void serialize(yasli::Archive& ar);
};

class Notepad : public QMainWindow
{
	Q_OBJECT

public:
	Notepad();

	private slots:
	void open();
	void save();

private:
	QTextEdit *textEdit;

	QAction *openAction;
	QAction *saveAction;
	QAction *exitAction;

	QMenu *fileMenu;

	QPropertyTree* propertyTree;

	Document document;
};
