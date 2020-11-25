/********************************************************************************
** Form generated from reading UI file 'Connector.ui'
**
** Created by: Qt User Interface Compiler version 5.12.9
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CONNECTOR_H
#define UI_CONNECTOR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ConnectorClass
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QLineEdit *leAddress;
    QPushButton *pbConnect;
    QPushButton *pbDisconnect;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QLineEdit *leIntervalMillisecond;
    QPushButton *pbStart;
    QPushButton *pbStop;
    QPushButton *pbHello;
    QSpacerItem *horizontalSpacer;
    QTextEdit *teSendData;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QPushButton *pbClear;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_4;
    QListWidget *lwSendData;
    QListWidget *lwRecvData;
    QLabel *labelStatus;

    void setupUi(QDialog *ConnectorClass)
    {
        if (ConnectorClass->objectName().isEmpty())
            ConnectorClass->setObjectName(QString::fromUtf8("ConnectorClass"));
        ConnectorClass->resize(595, 407);
        verticalLayout = new QVBoxLayout(ConnectorClass);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(ConnectorClass);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        leAddress = new QLineEdit(ConnectorClass);
        leAddress->setObjectName(QString::fromUtf8("leAddress"));

        horizontalLayout->addWidget(leAddress);

        pbConnect = new QPushButton(ConnectorClass);
        pbConnect->setObjectName(QString::fromUtf8("pbConnect"));

        horizontalLayout->addWidget(pbConnect);

        pbDisconnect = new QPushButton(ConnectorClass);
        pbDisconnect->setObjectName(QString::fromUtf8("pbDisconnect"));

        horizontalLayout->addWidget(pbDisconnect);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(ConnectorClass);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        leIntervalMillisecond = new QLineEdit(ConnectorClass);
        leIntervalMillisecond->setObjectName(QString::fromUtf8("leIntervalMillisecond"));
        leIntervalMillisecond->setMaximumSize(QSize(50, 16777215));

        horizontalLayout_2->addWidget(leIntervalMillisecond);

        pbStart = new QPushButton(ConnectorClass);
        pbStart->setObjectName(QString::fromUtf8("pbStart"));

        horizontalLayout_2->addWidget(pbStart);

        pbStop = new QPushButton(ConnectorClass);
        pbStop->setObjectName(QString::fromUtf8("pbStop"));

        horizontalLayout_2->addWidget(pbStop);

        pbHello = new QPushButton(ConnectorClass);
        pbHello->setObjectName(QString::fromUtf8("pbHello"));

        horizontalLayout_2->addWidget(pbHello);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout_2);

        teSendData = new QTextEdit(ConnectorClass);
        teSendData->setObjectName(QString::fromUtf8("teSendData"));

        verticalLayout->addWidget(teSendData);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label_3 = new QLabel(ConnectorClass);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_3->addWidget(label_3);

        pbClear = new QPushButton(ConnectorClass);
        pbClear->setObjectName(QString::fromUtf8("pbClear"));

        horizontalLayout_3->addWidget(pbClear);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        lwSendData = new QListWidget(ConnectorClass);
        lwSendData->setObjectName(QString::fromUtf8("lwSendData"));

        horizontalLayout_4->addWidget(lwSendData);

        lwRecvData = new QListWidget(ConnectorClass);
        lwRecvData->setObjectName(QString::fromUtf8("lwRecvData"));

        horizontalLayout_4->addWidget(lwRecvData);


        verticalLayout->addLayout(horizontalLayout_4);

        labelStatus = new QLabel(ConnectorClass);
        labelStatus->setObjectName(QString::fromUtf8("labelStatus"));

        verticalLayout->addWidget(labelStatus);


        retranslateUi(ConnectorClass);

        QMetaObject::connectSlotsByName(ConnectorClass);
    } // setupUi

    void retranslateUi(QDialog *ConnectorClass)
    {
        ConnectorClass->setWindowTitle(QApplication::translate("ConnectorClass", "Connector", nullptr));
        label->setText(QApplication::translate("ConnectorClass", "Address:", nullptr));
        pbConnect->setText(QApplication::translate("ConnectorClass", "Connect", nullptr));
        pbDisconnect->setText(QApplication::translate("ConnectorClass", "Disconnect", nullptr));
        label_2->setText(QApplication::translate("ConnectorClass", "Send data:", nullptr));
        leIntervalMillisecond->setText(QApplication::translate("ConnectorClass", "1000", nullptr));
        pbStart->setText(QApplication::translate("ConnectorClass", "Start", nullptr));
        pbStop->setText(QApplication::translate("ConnectorClass", "Stop", nullptr));
        pbHello->setText(QApplication::translate("ConnectorClass", "Hello", nullptr));
        label_3->setText(QApplication::translate("ConnectorClass", "Recieve data", nullptr));
        pbClear->setText(QApplication::translate("ConnectorClass", "Clear", nullptr));
        labelStatus->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ConnectorClass: public Ui_ConnectorClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CONNECTOR_H
