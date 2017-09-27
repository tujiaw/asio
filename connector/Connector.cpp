#include "Connector.h"
#include <QHostAddress>
#include <QMessageBox>
#include "addressbook.pb.h"
#include "ProtoHelp.h"

const QStringList STATELIST = QStringList() << "The socket is not connected."
	<< "The socket is performing a host name lookup."
	<< "The socket has started establishing a connection."
	<< "A connection is established."
	<< "The socket is bound to an address and port."
	<< "The socket is about to close (data may still be waiting to be written)."
	<< "For internal use only.";

Connector::Connector(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	connect(ui.pbConnect, &QPushButton::clicked, this, &Connector::onConnect);
	connect(ui.pbDisconnect, &QPushButton::clicked, this, &Connector::onDisconnect);
	connect(ui.pbStart, &QPushButton::clicked, this, &Connector::onStart);
	connect(ui.pbStop, &QPushButton::clicked, this, &Connector::onStop);
	connect(ui.pbClear, &QPushButton::clicked, this, &Connector::onRecvDataClear);
	connect(&socket_, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
	connect(&socket_, &QAbstractSocket::stateChanged, this, &Connector::onStateChanged);
	connect(&socket_, &QIODevice::readyRead, this, &Connector::onReadyRead);
	connect(ui.pbHello, &QPushButton::clicked, this, &Connector::onHello);

	ui.leAddress->setText("127.0.0.1:5566");
}

void Connector::onConnect()
{
	socket_.abort();
	QString address = ui.leAddress->text().trimmed();
	if (address.isEmpty()) {
		return;
	}

	QStringList strList = address.split(":");
	bool result = false;
	if (strList.size() == 2) {
		socket_.connectToHost(QHostAddress(strList[0]), strList[1].toInt());
	} else {
		socket_.connectToHost("127.0.0.1", address.toInt());
	}
}

void Connector::onDisconnect()
{
	socket_.close();
}

void Connector::onStart()
{
	QByteArray data(std::move(ui.teSendData->toPlainText().toUtf8()));
	socket_.write(data);
}

void Connector::onStop()
{

}

void Connector::onError(QAbstractSocket::SocketError socketError)
{
	if (socket_.errorString().isEmpty()) {
		ui.labelStatus->setText(QString("on error:%1").arg(socketError));
	} else {
		ui.labelStatus->setText(QString("on error:%1").arg(socket_.errorString()));
	}
}

void Connector::onStateChanged(QAbstractSocket::SocketState socketState)
{
	if (socketState < STATELIST.size() && socketState >= 0) {
		ui.labelStatus->setText(QString("on state changed:%1").arg(STATELIST[socketState]));
	} else {
		ui.labelStatus->setText(QString("on state changed:%1").arg(socketState));
	}
}

void Connector::onReadyRead()
{
	QByteArray data(std::move(socket_.readAll()));
	ui.lwRecvData->addItem(QString::fromUtf8(data));
	ui.lwRecvData->setCurrentRow(ui.lwRecvData->count() - 1);
	qDebug() << "recv length:" << data.size();
}

void Connector::onRecvDataClear()
{
	ui.lwRecvData->clear();
}

void Connector::onHello()
{
	Test::Hello hello;
	hello.set_name("tujiaw");
	hello.set_id(123);
	hello.set_address("shanghai pudong");
	std::string buf = ProtoHelp::encode(hello);
	socket_.write(buf.c_str(), buf.size());
}