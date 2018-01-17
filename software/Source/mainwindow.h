#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QSerialPort>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void saveSession();
	void restoreSession();

private slots:
	void onCursorPositionChanged();
	void on_actionOpen_triggered();
	void on_actionSave_triggered();
	void on_actionSaveAs_triggered();
	void on_actionAboutQt_triggered();
	void on_actionAbout_triggered();
	void on_pushUpdate_clicked();

private:
	Ui::MainWindow *ui;
	QString mFileName;

	void enumeratePorts();
	void enumerateBaudRates();

	void openBinary(QString filename);
	void openIntelHex(QString filename);
	void saveBinary(QString filename);
	void saveIntelHex(QString filename);
	void setFileName(QString filename = "");

	void closeEvent(QCloseEvent *event);
};

#endif // MAINWINDOW_H
