#include "pch.h"
#include "notepad.h"
#include "yasli/Enum.h"
#include "yasli/STL.h"

void Entry::serialize(yasli::Archive& ar)
{
	ar(name, "name", "Name");
	ar(age, "age", "Age");
	ar(position, "position", "Position");
}

void Document::serialize(yasli::Archive& ar)
{
	ar(entries, "entries", "Entries");
}

YASLI_ENUM_BEGIN(Position, "Position")
YASLI_ENUM(ENGINEER, "engineer", "Engineer")
YASLI_ENUM(MANAGER, "manager", "Manager")
YASLI_ENUM_END()

Notepad::Notepad()
{
	openAction = new QAction(tr("&Load"), this);
	saveAction = new QAction(tr("&Save"), this);
	exitAction = new QAction(tr("E&xit"), this);

	connect(openAction, SIGNAL(triggered()), this, SLOT(open()));
	connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
	connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

	fileMenu = menuBar()->addMenu(tr("&File"));
	fileMenu->addAction(openAction);
	fileMenu->addAction(saveAction);
	fileMenu->addSeparator();
	fileMenu->addAction(exitAction);

	textEdit = new QTextEdit;
	propertyTree = new QPropertyTree();
	propertyTree->setUndoEnabled(true, false);
	propertyTree->attach(yasli::Serializer(document));
	propertyTree->expandAll();
	setCentralWidget(propertyTree);

	setWindowTitle(tr("Notepad"));
}

void Notepad::open()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",
		tr("Text Files (*.txt);;C++ Files (*.cpp *.h)"));

	if (!fileName.isEmpty()) {
		QFile file(fileName);
		if (!file.open(QIODevice::ReadOnly)) {
			QMessageBox::critical(this, tr("Error"), tr("Could not open file"));
			return;
		}
		QTextStream in(&file);
		textEdit->setText(in.readAll());
		file.close();
	}
}

void Notepad::save()
{

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "",
		tr("Text Files (*.txt);;C++ Files (*.cpp *.h)"));

	if (!fileName.isEmpty()) {
		QFile file(fileName);
		if (!file.open(QIODevice::WriteOnly)) {
			// error message
		}
		else {
			QTextStream stream(&file);
			stream << textEdit->toPlainText();
			stream.flush();
			file.close();
		}
	}
}


int NotepadTest(int argc, char** argv)
{
	QApplication app{ argc, argv };

	Notepad notepad;
	notepad.show();

	return app.exec();
}
