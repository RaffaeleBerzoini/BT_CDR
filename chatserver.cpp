#include "chatserver.h"
#include <iostream>
#include <QtBluetooth/qbluetoothserver.h>
#include <QtBluetooth/qbluetoothsocket.h>
#include <QString>

//! [Service UUID]
static const QLatin1String serviceUuid("e8e10f95-1a70-4b27-9ccf-02010264e9c8");
//! [Service UUID]

ChatServer::ChatServer(QObject *parent)
    :   QObject(parent)
{
    std::cout << "Constructor\n";
}

ChatServer::~ChatServer()
{
    stopServer();
}

void ChatServer::startServer(const QBluetoothAddress& localAdapter)
{
    std::cout << "HERE1\n";
    if (rfcommServer){
        std::cout << "HERE2\n";
        return;
    }

    //! [Create the server]
    std::cout << "HERE3\n";
    rfcommServer = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this);
    connect(rfcommServer, &QBluetoothServer::newConnection,
            this, QOverload<>::of(&ChatServer::clientConnected));
    connect(this, &ChatServer::messageReceived, this, &ChatServer::printMessage);
    std::cout << "HERE4\n";
    bool result = rfcommServer->listen(localAdapter);
    if (!result) {
        qWarning() << "Cannot bind chat server to" << localAdapter.toString();
        return;
    }
    std::cout << "HERE5\n";
    //! [Create the server]

    //serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceRecordHandle, (uint)0x00010010);

    QBluetoothServiceInfo::Sequence profileSequence;
    QBluetoothServiceInfo::Sequence classId;
    classId << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::SerialPort));
    classId << QVariant::fromValue(quint16(0x100));
    profileSequence.append(QVariant::fromValue(classId));
    serviceInfo.setAttribute(QBluetoothServiceInfo::BluetoothProfileDescriptorList,
                             profileSequence);

    classId.clear();
    classId << QVariant::fromValue(QBluetoothUuid(serviceUuid));
    classId << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::SerialPort));

    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceClassIds, classId);

    //! [Service name, description and provider]
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceName, tr("Bt Chat Server"));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceDescription,
                             tr("Example bluetooth chat server"));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ServiceProvider, tr("qt-project.org"));
    //! [Service name, description and provider]

    //! [Service UUID set]
    serviceInfo.setServiceUuid(QBluetoothUuid(serviceUuid));
   // QString foo = QString::fromLatin1(serviceUuid);
    std::cout << qPrintable(serviceUuid)<< std::endl;
    //! [Service UUID set]

    //! [Service Discoverability]
    QBluetoothServiceInfo::Sequence publicBrowse;
    publicBrowse << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::PublicBrowseGroup));
    serviceInfo.setAttribute(QBluetoothServiceInfo::BrowseGroupList,
                             publicBrowse);
    //! [Service Discoverability]

    //! [Protocol descriptor list]
    QBluetoothServiceInfo::Sequence protocolDescriptorList;
    QBluetoothServiceInfo::Sequence protocol;
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::L2cap));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    protocol.clear();
    protocol << QVariant::fromValue(QBluetoothUuid(QBluetoothUuid::Rfcomm))
             << QVariant::fromValue(quint8(rfcommServer->serverPort()));
    protocolDescriptorList.append(QVariant::fromValue(protocol));
    serviceInfo.setAttribute(QBluetoothServiceInfo::ProtocolDescriptorList,
                             protocolDescriptorList);
    //! [Protocol descriptor list]

    //! [Register service]
    serviceInfo.registerService(localAdapter);
    //! [Register service]
}

void ChatServer::printMessage(const QString &sender, const QString &message){
    std::cout << "Print Message" << std::endl;
    std::cout << message.toStdString() << std::endl;
    start = true;
}

//! [stopServer]
void ChatServer::stopServer()
{
    // Unregister service
    serviceInfo.unregisterService();

    // Close sockets
    qDeleteAll(clientSockets);

    // Close server
    delete rfcommServer;
    rfcommServer = nullptr;
}
//! [stopServer]

//! [sendMessage]
void ChatServer::sendMessage(const QString &message)
{
    QByteArray text = message.toUtf8() + '\n';

    for (QBluetoothSocket *socket : qAsConst(clientSockets))
        socket->write(text);
}
//! [sendMessage]

//! [clientConnected]
void ChatServer::clientConnected()
{
    QBluetoothSocket *socket = rfcommServer->nextPendingConnection();
    if (!socket)
        return;

    std::cout << "clientConnected" << std::endl;
    connect(socket, &QBluetoothSocket::readyRead, this, &ChatServer::readSocket);
    connect(socket, &QBluetoothSocket::disconnected, this, QOverload<>::of(&ChatServer::clientDisconnected));
    clientSockets.append(socket);
    emit clientConnected(socket->peerName());
}
//! [clientConnected]

//! [clientDisconnected]
void ChatServer::clientDisconnected()
{
    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket)
        return;

    emit clientDisconnected(socket->peerName());

    clientSockets.removeOne(socket);

    socket->deleteLater();
}
//! [clientDisconnected]

//! [readSocket]
void ChatServer::readSocket()
{
    QBluetoothSocket *socket = qobject_cast<QBluetoothSocket *>(sender());
    if (!socket)
        return;

    std::cout << "readSocket" << std::endl;
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        std::cout << "Reading line" << std::endl;
        emit messageReceived(socket->peerName(),
                             QString::fromUtf8(line.constData(), line.length()));
        
    }
}
//! [readSocket]