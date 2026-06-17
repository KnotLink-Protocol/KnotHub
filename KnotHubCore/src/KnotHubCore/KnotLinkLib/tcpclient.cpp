#include "tcpclient.h"
#include <QDebug>
#include <QDataStream>
#include <QtEndian>

const quint32 MAX_MSG_SIZE = 16 * 1024 * 1024; // 16MB

TcpClient::TcpClient(QObject *parent) : QObject(parent) {
    tcpSocket = new QTcpSocket(this);
    heartBeatTimer = new QTimer(this);
    heartBeatTimer->setInterval(180000); // 3分钟
}

TcpClient::~TcpClient() {
    delete tcpSocket;
    // heartBeatTimer 由 Qt 父子关系自动删除，无需手动 delete
}

void TcpClient::connectToServer(const QString &ip, uint16_t port) {
    tcpSocket->connectToHost(QHostAddress(ip), port);

    if (!tcpSocket->waitForConnected(3000)) {
        qDebug() << "连接失败：" << tcpSocket->errorString();
        return;
    }

    connect(tcpSocket, &QTcpSocket::connected, this, &TcpClient::socketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpClient::socketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpClient::readData);
    connect(tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &TcpClient::handleError);

    connect(heartBeatTimer, &QTimer::timeout, this, &TcpClient::sendHeartbeat);
    heartBeatTimer->start();
}

void TcpClient::socketConnected() {
    qDebug() << "已连接到服务器";
    sendHeartbeat(); // 连接成功后立即发一次心跳（可选）
    emit connected();
}

void TcpClient::socketDisconnected() {
    qDebug() << "与服务器断开连接";
    heartBeatTimer->stop();  // 只停止，不删除
    readBuffer.clear();      // 清空缓冲区
    emit disconnected();
}

void TcpClient::handleError(QAbstractSocket::SocketError socketError) {
    qDebug() << "Socket错误：" << socketError << " - " << tcpSocket->errorString();
}

// ------------------------------------------------------------
// 发送数据（带长度前缀）
// ------------------------------------------------------------
void TcpClient::writeWithLengthPrefix(const QByteArray &data) {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_0);
    out << (quint32)data.size();
    block.append(data);
    tcpSocket->write(block);
}

void TcpClient::sendData(const QByteArray &data) {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        writeWithLengthPrefix(data);
        qDebug() << "发送业务数据：" << data.size() << "字节";
    } else {
        qDebug() << "无法发送数据，连接未建立";
    }
}

void TcpClient::sendHeartbeat() {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        writeWithLengthPrefix("heartbeat");
        qDebug() << "发送心跳包";
    } else {
        qDebug() << "无法发送心跳，连接已断开";
    }
}

// ------------------------------------------------------------
// 接收数据：追加到缓冲区，循环解析完整消息
// ------------------------------------------------------------
void TcpClient::readData() {
    if (!tcpSocket) return;

    QByteArray newData = tcpSocket->readAll();
    if (newData.isEmpty()) return;

    readBuffer.append(newData);
    processBuffer();
}

void TcpClient::processBuffer() {
    while (true) {
        if (readBuffer.size() < 4) break; // 长度字段未完整

        quint32 len = qFromBigEndian<quint32>((const uchar*)readBuffer.constData());

        if (len == 0 || len > MAX_MSG_SIZE) {
            qDebug() << "无效消息长度：" << len << "，断开连接";
            tcpSocket->disconnectFromHost();
            return;
        }

        if (readBuffer.size() < (int)(len + 4)) break; // 消息体未完整

        QByteArray msg = readBuffer.mid(4, len);
        readBuffer.remove(0, len + 4);

        // 处理消息
        if (msg == "heartbeat_response") {
            qDebug() << "收到心跳响应，忽略";
        } else {
            emit receivedData(msg);
            qDebug() << "收到应用消息：" << msg.size() << "字节";
        }
        // 继续循环，可能还有更多完整消息
    }
}
