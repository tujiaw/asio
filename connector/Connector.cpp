#include "Connector.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QTimer>
#include "asio/proto/pb_base.pb.h"
#include "asio/msgclient.h"

Connector::Connector(QWidget *parent)
	: QDialog(parent), id_(100), timer_(new QTimer(this))
	, conn_("127.0.0.1:5566", 30000)
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
    //sendMsg();
    //sendMsg();
    //sendMsg();
}

void Connector::sendMsg()
{
	++id_;
    std::string content = (QString::number(id_) + ":" + ui.teSendData->toPlainText()).toStdString();
	MessagePtr msgPtr(new PbBase::EchoReq());
    PbBase::EchoReq *req = (PbBase::EchoReq*)msgPtr.get();
    req->set_content(content);

    conn_.postMessage(msgPtr, [content](int error, const PackagePtr &reqMsgPtr, const PackagePtr &rspMsgPtr) {
        if (rspMsgPtr) {
            PbBase::EchoRsp *msg = static_cast<PbBase::EchoRsp*>(rspMsgPtr->msgPtr.get());
            QByteArray xx = QString::fromStdString(msg->content()).toLocal8Bit();
            if (msg->content() == content) {
                std::string id = content.substr(0, content.find(':'));
                LOG(INFO) << id << ",ok";
            } else {
                LOG(ERROR) << "failed";
            }
        }
    });

    //MessagePtr rsp;
    //if (0 == conn_.sendMessage(msgPtr, rsp)) {
    //    PbBase::EchoRsp *msg = static_cast<PbBase::EchoRsp*>(rsp.get());
    //    std::string resContent = msg->content();
    //    QByteArray xx = QString::fromStdString(resContent).toLocal8Bit();
    //    if (resContent == content) {
    //        std::string id = content.substr(0, content.find(':'));
    //        //ui.lwRecvData->addItem(QString::fromStdString(id));
    //        LOG(INFO) << id << ",ok";
    //    }
    //}
}
