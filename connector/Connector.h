#pragma once

#include <QtWidgets/QDialog>
#include "ui_Connector.h"
#include <QTcpSocket>

class QTimer;
class Connector : public QDialog
{
	Q_OBJECT

public:
	Connector(QWidget *parent = Q_NULLPTR);

	void onConnect();
	void onDisconnect();
	void onStart();
	void onStop();
	void onStateChanged(QAbstractSocket::SocketState socketState);
	void onReadyRead();
	void onRecvDataClear();
	void onHello();

	void sendMsg();

public slots:
	void onError(QAbstractSocket::SocketError socketError);
	void onTimer();

private:
	Ui::ConnectorClass ui;
	QTcpSocket socket_;
	int id_;
	QTimer *timer_;
};
