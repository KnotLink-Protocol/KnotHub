#include "tcpclient.h"

TcpClient::TcpClient(QObject *parent) : QObject(parent) {
    tcpSocket = new QTcpSocket(this);
    heartBeatTimer = new QTimer(this);
    heartBeatTimer->setInterval(180000);  // 设置定时时间为3分钟
}

TcpClient::~TcpClient() {
    delete tcpSocket;
    if (heartBeatTimer != nullptr) {
        delete heartBeatTimer;
    }
}

void TcpClient::connectToServer(const QString &ip, uint16_t port) {
    tcpSocket->connectToHost(QHostAddress(ip), port);

    if (!tcpSocket->waitForConnected(3000)) {
        qDebug() << "连接失败：" << tcpSocket->errorString();
        return;
    }

//    sendHeartbeat();

    // 连接定时器信号到槽函数
    connect(heartBeatTimer, &QTimer::timeout, this, &TcpClient::sendHeartbeat);

    // 启动定时器
    heartBeatTimer->start();

    connect(tcpSocket, &QTcpSocket::connected, this, &TcpClient::socketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpClient::socketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::readData);
//    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &TcpClient::handleError);
}

void TcpClient::socketConnected() {
    sendHeartbeat();
    emit connected();
}

void TcpClient::socketDisconnected() {
    emit disconnected();

    // 停止心跳定时器
    heartBeatTimer->stop();

    // 删除定时器以释放内存
    delete heartBeatTimer;
    heartBeatTimer = nullptr;
}

void TcpClient::readData() {
    if (tcpSocket->bytesAvailable() > 0) {
        QByteArray data = tcpSocket->readAll();
        qDebug() << "收到数据：" << data;
        if(data == "heartbeat_response")
            return;
        emit receivedData(data);
    }
}

void TcpClient::handleError(QAbstractSocket::SocketError socketError) {
    qDebug() << "错误：" << socketError << " - " << tcpSocket->errorString();
}

void TcpClient::sendHeartbeat() {
    QByteArray heartbeatData;
    heartbeatData.append("heartbeat");

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qint64 bytesWritten = tcpSocket->write(heartbeatData);
        if (bytesWritten == -1) {
            qDebug() << "发送心跳包失败：" << tcpSocket->errorString();
        } else {
            qDebug() << "发送心跳包成功";
        }
    } else {
        qDebug() << "无法发送心跳包，连接已断开。";
    }
}

void TcpClient::sendData(const QByteArray &data) {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        if (tcpSocket->write(data) == -1) {
            qDebug() << "发送数据失败：" << tcpSocket->errorString();
        }
    } else {
        qDebug() << "无法发送数据，连接已断开。";
    }
}
