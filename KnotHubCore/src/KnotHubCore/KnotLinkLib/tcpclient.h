#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QByteArray>

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void connectToServer(const QString &ip, uint16_t port);
    void sendData(const QByteArray &data);   // 业务数据（自动加前缀）

signals:
    void connected();
    void disconnected();
    void receivedData(const QByteArray &data); // 完整消息体（不含前缀）

private slots:
    void socketConnected();
    void socketDisconnected();
    void readData();
    void handleError(QAbstractSocket::SocketError socketError);
    void sendHeartbeat();

private:
    QTcpSocket *tcpSocket;
    QTimer *heartBeatTimer;
    QByteArray readBuffer;    // 累积接收缓冲区

    // 辅助函数
    void writeWithLengthPrefix(const QByteArray &data);
    void processBuffer();
};

#endif // TCPCLIENT_H
