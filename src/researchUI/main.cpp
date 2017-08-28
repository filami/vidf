#include "pch.h"

#include <QApplication>
#include <QTextEdit>

int main(int argc, char* argv[])
{
	vidf::SetCurrentDirectory("../../");
	QApplication app(argc, argv);

	QTextEdit textEdit;
	textEdit.show();

	return app.exec();
}
