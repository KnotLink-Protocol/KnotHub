#ifndef NODELOADER_H
#define NODELOADER_H

#include <QObject>
#include <QProcess>
#include <windows.h>

class NodeLoader : public QObject
{
    Q_OBJECT
public:
    explicit NodeLoader(QObject *parent = nullptr);
    ~NodeLoader();

    void start(const QString &program, const QStringList &arguments);
    void stop();
    bool statuscheck() const;   // true=运行中, false=未运行

signals:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error, const QString &errorString);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onReadyRead();         // 新增：读取并打印进程输出

private:
    QProcess *m_process;
    bool m_isRunning;

    // ── Windows Job Object（防止孤儿进程） ──
    static HANDLE s_jobHandle;
    static void ensureJobObject();
};

#endif // NODELOADER_H
