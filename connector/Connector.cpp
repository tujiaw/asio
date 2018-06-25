#include "Connector.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QTimer>
#include "asio/proto/pb_base.pb.h"
#include "asio/msgclient.h"

Connector::Connector(QWidget *parent)
	: QDialog(parent), id_(100), timer_(new QTimer(this))
	, conn_("127.0.0.1:5566", 3)
{
	ui.setupUi(this);

	connect(ui.pbConnect, &QPushButton::clicked, this, &Connector::onConnect);
	connect(ui.pbDisconnect, &QPushButton::clicked, this, &Connector::onDisconnect);
	connect(ui.pbStart, &QPushButton::clicked, this, &Connector::onStart);
	connect(ui.pbStop, &QPushButton::clicked, this, &Connector::onStop);
	connect(ui.pbClear, &QPushButton::clicked, this, &Connector::onRecvDataClear);
	connect(ui.pbHello, &QPushButton::clicked, this, &Connector::onHello);
	connect(timer_, &QTimer::timeout, this, &Connector::onTimer);

	ui.leAddress->setText("127.0.0.1:5566");
	ui.teSendData->setText("hello, world!");
}

void Connector::onConnect()
{
	conn_.start();
}

void Connector::onDisconnect()
{
	conn_.stop();
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

void Connector::onTimer()
{
	sendMsg();
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
	MessagePtr msgPtr(new PbBase::HelloReq());
	PbBase::HelloReq *hello = (PbBase::HelloReq*)msgPtr.get();
	hello->set_name("tujiaw");
	hello->set_id(id_);
	hello->set_address((QString::number(id_) + ui.teSendData->toPlainText()).toStdString());
	//conn_.postMessage(msgPtr, [](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
	//	if (rspMsgPtr) {
	//		
	//	}
	//});
	MessagePtr rsp;
	if (0 == conn_.sendMessage(msgPtr, rsp)) {
		PbBase::HelloRsp *msg = static_cast<PbBase::HelloRsp*>(rsp.get());
		ui.lwRecvData->addItem(QString::fromStdString(msg->hello()));
	}
}
