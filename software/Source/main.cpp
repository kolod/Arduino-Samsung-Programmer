#include "mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QtCore>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("Arduino Samsung Programmer");
	a.setOrganizationName("Alexandr Kolodkin");
	a.setStyle(QStyleFactory::create("Fusion"));

	QTranslator qtTranslator;
	qtTranslator.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	a.installTranslator(&qtTranslator);

	MainWindow w;
	w.show();
	w.restoreSession();

	return a.exec();
}
