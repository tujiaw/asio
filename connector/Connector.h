#pragma once

#include <QtWidgets/QDialog>
#include "ui_Connector.h"
#include <QTcpSocket>

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

public slots:
	void onError(QAbstractSocket::SocketError socketError);

private:
	Ui::ConnectorClass ui;
	QTcpSocket socket_;
};
