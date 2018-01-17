#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>
#include <QSerialPortInfo>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, mFileName("")
{
	ui->setupUi(this);
	enumerateBaudRates();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::saveSession()
{
	QSettings settings;

	settings.setValue("geometry"     , saveGeometry());
	settings.setValue("state"        , saveState());
	settings.setValue("port"         , ui->inputPort->currentText());
	settings.setValue("speed"        , ui->inputSpeed->currentText());
}

void MainWindow::restoreSession()
{
	QSettings settings;

	enumeratePorts();

	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("state").toByteArray());
	ui->inputPort->setCurrentText(settings.value("port").toString());
	ui->inputSpeed->setCurrentText(settings.value("speed", "115200").toString());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	saveSession();
	event->accept();
}

void MainWindow::enumeratePorts()
{
	int id = 0;
	ui->inputPort->clear();
	foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
		QString tooltip =
		  QObject::tr(
			"Port: %1\n"
			"Location: %2\n"
			"Description: %3\n"
			"Manufacturer: %4\n"
			"Vendor Identifier: %5\n"
			"Product Identifier: %6"
		  )
		  .arg(info.portName())
		  .arg(info.systemLocation())
		  .arg(info.description())
		  .arg(info.manufacturer())
		  .arg(info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString())
		  .arg(info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString());

		ui->inputPort->addItem(info.portName());
		ui->inputPort->setItemData(id, QVariant(tooltip), Qt::ToolTipRole);
		id++;
	}
}

void MainWindow::enumerateBaudRates()
{
	ui->inputSpeed->clear();
	foreach (qint32 BaudRate, QSerialPortInfo::standardBaudRates()) {
		ui->inputSpeed->addItem(QString("%1").arg(BaudRate), QVariant(BaudRate));
	}
}

void MainWindow::openBinary(QString filename)
{
	QHexDocument *oldDocument = ui->hexedit->document();
	if (oldDocument != nullptr) oldDocument->deleteLater();
	QHexDocument *document = QHexDocument::fromFile(filename);
	document->highlightForeRange(0, document->length(), QColor(Qt::black));
	ui->hexedit->setDocument(document);
}

void MainWindow::openIntelHex(QString filename)
{
	QHexDocument* document = QHexDocument::fromMemory(QByteArray());
	QFile hexfile(filename);
	bool ok = true;
	int next = 0;

	if (!hexfile.open(QIODevice::ReadOnly | QIODevice::Text)) return;

	while (!hexfile.atEnd()) {
		QByteArray line = hexfile.readLine().simplified();
		if (!line.startsWith(':')) continue;
		if (line.length() < 9) continue;

		QByteArray data = QByteArray::fromHex(line.mid(1));
		if (data.length() == 0) {
			ok = false;
			break;
		}

		quint8 checksumm = 0;
		for (int i = 0; i < data.length(); i++)
			checksumm += static_cast<quint8>(data.at(i));
		if (checksumm != 0) {
			ok = false;
			break;
		}

		// Read record length
		auto count = data.at(0);

		// Read address
		int address = static_cast<quint8>(data.at(1)) * 256 + static_cast<quint8>(data.at(2));

		// Read record type
		auto recordtype = data.at(3);

		if (recordtype == 0) {
			if (next != address) {
				document->replace(next, QByteArray(address - next, 0xFF));
				document->highlightFore(next, address - 1, QColor(Qt::gray));
			}
			document->replace(address, data.mid(4, count));
			document->highlightForeRange(address, count, QColor(Qt::black));
			next = address + count;
		}

		if (!ok) break;
	}

	if (ok) {
		int size = document->length();
		int len = 256 - (size % 256);

		if (len) {
			document->insert(size, QByteArray(len, 0xFF));
			document->highlightForeRange(size, len, QColor(Qt::gray));
		}

		auto *oldDocument = ui->hexedit->document();
		if (oldDocument != nullptr) oldDocument->deleteLater();
		ui->hexedit->setDocument(document);
	} else {
		qDebug() << "Error while reading Intel hex file";
	}
}

void MainWindow::saveBinary(QString filename)
{
	QFile hexfile(filename);
	if (hexfile.open(QIODevice::WriteOnly | QIODevice::Truncate))
		ui->hexedit->document()->saveTo(&hexfile);
	hexfile.close();
}

void MainWindow::saveIntelHex(QString filename)
{
	QFile hexfile(filename);

	if (!hexfile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
		QMessageBox::critical(
		  this,
		  tr("File saving failed!"),
		  tr("Can't save data to %1.\n File is blocked or write pemission missed.").arg(mFileName),
		  QMessageBox::Close
		);

		return;
	}

//	hexfile << ":020000020000FC\n";



//	hexfile << ":00000001FF\n";

	hexfile.close();
}

void MainWindow::setFileName(QString filename)
{
	QFileInfo info(filename);

	if (filename.simplified().isEmpty()) {
		mFileName = "";
		setWindowTitle(tr("Arduino Based Samsung Programmer"));
	} else {
		if (info.exists()) {
			mFileName = QFileInfo(filename).canonicalFilePath();
		} else {
			mFileName = QFileInfo(filename).absoluteFilePath();
		}

		setWindowTitle(mFileName + " - " + tr("Arduino Based Samsung Programmer"));
	}
}

void MainWindow::onCursorPositionChanged()
{
	auto *document = ui->hexedit->document();
	auto cursor = document->cursor();
	int available = document->length() - cursor->offset();
	int count = (available > 7) ? 8 : (available > 3) ? 4 :(available > 1) ? 2 : 1;

	typedef union {
		qint8     int8;
		qint16    int16;
		qint32    int32;
		qint64    int64;
		quint8    uint8;
		quint16   uint16;
		quint32   uint32;
		quint64   uint64;
	} data_t;

	data_t *data = (data_t*) ui->hexedit->document()->read(cursor->offset(), count).data();

	ui->tableDataView->setItem(0, 1, new QTableWidgetItem(QString::number(data->int8)));
	ui->tableDataView->setItem(4, 1, new QTableWidgetItem(QString::number(data->uint8)));

	if (count >= 2) {
		ui->tableDataView->setItem(1, 1, new QTableWidgetItem(QString::number(data->int16)));
		ui->tableDataView->setItem(5, 1, new QTableWidgetItem(QString::number(data->uint16)));
	} else {
		ui->tableDataView->setItem(1, 1, new QTableWidgetItem());
		ui->tableDataView->setItem(5, 1, new QTableWidgetItem());
	}

	if (count >= 4) {
		ui->tableDataView->setItem(2, 1, new QTableWidgetItem(QString::number(data->int32)));
		ui->tableDataView->setItem(6, 1, new QTableWidgetItem(QString::number(data->uint32)));
	} else {
		ui->tableDataView->setItem(2, 1, new QTableWidgetItem());
		ui->tableDataView->setItem(6, 1, new QTableWidgetItem());
	}

	if (count >= 8) {
		ui->tableDataView->setItem(3, 1, new QTableWidgetItem(QString::number(data->int64)));
		ui->tableDataView->setItem(7, 1, new QTableWidgetItem(QString::number(data->uint64)));
	} else {
		ui->tableDataView->setItem(3, 1, new QTableWidgetItem());
		ui->tableDataView->setItem(7, 1, new QTableWidgetItem());
	}
}

void MainWindow::on_actionOpen_triggered()
{
	QSettings settings;

	auto filename = QFileDialog::getOpenFileName(
	  this,
	  tr("Open Firmware"),
	  settings.value("lastdir", "/").toString(),
	  tr("All Firmware Files(*.bin *.ihex *.hex);;Binary Image Files(*.bin);;Intel Hex Image Files(*.hex *.ihex)")
	);

	QFileInfo info(filename);

	if (!info.isFile()) return;
	if (!info.isReadable()) return;

	settings.setValue("lastdir", info.dir().canonicalPath());

	if (filename.endsWith("hex")) {
		openIntelHex(filename);
	} else {
		openBinary(filename);
	}

	connect(ui->hexedit->document()->cursor(), &QHexCursor::offsetChanged, this, &MainWindow::onCursorPositionChanged);

	setFileName(filename);
}

void MainWindow::on_actionSave_triggered()
{
	if (mFileName.isEmpty()) on_actionSaveAs_triggered();

	if (!QFileInfo(mFileName).isWritable()) {
		if (QMessageBox::question(
		  this,
		  tr("File isn't writable"),
		  tr("This file isn't writable.Do you want to save your changes to another file?\n")
		) == QMessageBox::Ok) {
			on_actionSaveAs_triggered();
		}
	}

	if (mFileName.endsWith(".hex")) {
		saveIntelHex(mFileName);
	} else {
		saveBinary(mFileName);
	}
}

void MainWindow::on_actionSaveAs_triggered()
{
	QFileInfo old(mFileName);

	auto filename = QFileDialog::getSaveFileName(
	  this,
	  tr("Open Firmware"),
	  old.dir().canonicalPath() + "/" + old.completeBaseName(),
	  tr("Binary Image Files(*.bin);;Intel Hex Image Files(*.hex)")
	);

	if (filename.isEmpty()) return;

	QFileInfo info(filename);

	if (info.exists())
		if (!info.isFile() || !info.isWritable()) return;

	setFileName(filename);

	if (mFileName.endsWith(".hex")) {
		saveIntelHex(mFileName);
	} else {
		saveBinary(mFileName);
	}
}

void MainWindow::on_actionAboutQt_triggered()
{
	qApp->aboutQt();
}

void MainWindow::on_actionAbout_triggered()
{
	QMessageBox::about(
	  this,
	  tr("Arduino Based Samsung Progammer"),
	  tr("")
	);
}

void MainWindow::on_pushUpdate_clicked()
{
	enumeratePorts();
}
