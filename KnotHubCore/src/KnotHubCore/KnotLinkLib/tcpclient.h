#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

class TcpClient : public QObject {
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void connectToServer(const QString &ip, uint16_t port);
    void sendData(const QByteArray &data);

signals:
    void connected();
    void disconnected();
    void receivedData(const QByteArray &data);

private slots:
    void socketConnected();
    void socketDisconnected();
    void readData();
    void handleError(QAbstractSocket::SocketError socketError);
    void sendHeartbeat();  // 新增槽函数声明

private:
    QTcpSocket *tcpSocket;
    QTimer *heartBeatTimer;  // 新增心跳定时器成员变量
};

#endif // TCPCLIENT_H
