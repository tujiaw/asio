#include "Connector.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QTimer>
#include "addressbook.pb.h"
#include "ProtoHelp.h"
#include "Buffer.h"
const QStringList STATELIST = QStringList() << "The socket is not connected."
	<< "The socket is performing a host name lookup."
	<< "The socket has started establishing a connection."
	<< "A connection is established."
	<< "The socket is bound to an address and port."
	<< "The socket is about to close (data may still be waiting to be written)."
	<< "For internal use only.";

Connector::Connector(QWidget *parent)
	: QDialog(parent), id_(100), timer_(new QTimer(this))
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
	connect(timer_, &QTimer::timeout, this, &Connector::onTimer);

	ui.leAddress->setText("127.0.0.1:5566");
	ui.teSendData->setText("hello, world!");
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
	int ms = ui.leIntervalMillisecond->text().toInt();
	if (ms <= 0) {
		return;
	}

	if (timer_->interval() != ms) {
		timer_->setInterval(ms);
		timer_->stop();
	}
	timer_->start();
}

void Connector::onStop()
{
	timer_->stop();
}

void Connector::onError(QAbstractSocket::SocketError socketError)
{
	if (socket_.errorString().isEmpty()) {
		ui.labelStatus->setText(QString("on error:%1").arg(socketError));
	} else {
		ui.labelStatus->setText(QString("on error:%1").arg(socket_.errorString()));
	}
}

void Connector::onTimer()
{
	sendMsg();
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
	std::string str(data.data(), data.size());
	Buffer buf;
	buf.append(str);
	PackagePtr pacPtr = ProtoHelp::decode(buf);
	if (pacPtr && pacPtr->msgPtr->GetTypeName() == Test::HelloRsp::default_instance().GetTypeName()) {
		Test::HelloRsp *rsp = static_cast<Test::HelloRsp*>(pacPtr->msgPtr.get());
		if (rsp) {
			if (ui.lwRecvData->count() > 3000) {
				ui.lwRecvData->clear();
			}
			ui.lwRecvData->addItem(QString::fromStdString(rsp->hello()));
			ui.lwRecvData->scrollToBottom();
		}
	} else {
		ui.labelStatus->setText("decode failed");
	}
	qDebug() << "recv length:" << data.size();
}

void Connector::onRecvDataClear()
{
	ui.lwRecvData->clear();
}

void Connector::onHello()
{
	sendMsg();
}

void Connector::sendMsg()
{
	++id_;
	MessagePtr msgPtr(new Test::HelloReq());
	Test::HelloReq *hello = (Test::HelloReq*)msgPtr.get();
	hello->set_name("tujiaw");
	hello->set_id(id_);
	hello->set_address((QString::number(id_) + ui.teSendData->toPlainText()).toStdString());

	PackagePtr pacPtr(new Package());
	pacPtr->id = id_;
	pacPtr->typeName = hello->GetTypeName();
	pacPtr->typeNameLen = hello->GetTypeName().length();
	pacPtr->msgPtr = msgPtr;

	std::string buf = ProtoHelp::encode(pacPtr);
	socket_.write(buf.c_str(), buf.size());
}
