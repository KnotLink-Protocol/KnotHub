#ifndef NODELOADER_H
#define NODELOADER_H

#include <QObject>
#include <QProcess>
#include <QtDebug>
#include <QThread>

class NodeLoader : public QObject
{
    Q_OBJECT
public:
    explicit NodeLoader(QObject *parent = nullptr);
    ~NodeLoader();

    void start(const QString &program, const QStringList &arguments);
    void stop();
    bool statuscheck() const;  // true=运行中, false=未运行

signals:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void processError(QProcess::ProcessError error, const QString &errorString);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    QProcess *m_process;
    bool m_isRunning;
};

#endif // NODELOADER_H
