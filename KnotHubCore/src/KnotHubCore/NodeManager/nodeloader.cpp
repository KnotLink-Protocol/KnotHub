#include "nodeloader.h"
#include <QDebug>

NodeLoader::NodeLoader(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isRunning(false)
{
    // 合并标准输出和错误，便于统一处理
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // 连接信号
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NodeLoader::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &NodeLoader::onProcessError);
    // 新增：连接 readyRead，用于实时打印输出
    connect(m_process, &QProcess::readyRead,
            this, &NodeLoader::onReadyRead);
}

NodeLoader::~NodeLoader()
{
    if (m_isRunning) {
        qWarning() << "Destructor called while process is still running, stopping...";
        stop();
    }
}

void NodeLoader::start(const QString &program, const QStringList &arguments)
{
    if (m_isRunning) {
        qWarning() << "Process already running, ignoring start request";
        return;
    }

    m_process->start(program, arguments);
    bool started = m_process->waitForStarted(3000);
    if (started) {
        m_isRunning = true;
        qDebug() << "Started process:" << program << arguments;
    } else {
        qCritical() << "Failed to start process:" << m_process->errorString();
        m_isRunning = false;
    }
}

void NodeLoader::stop()
{
    if (!m_isRunning)
        return;

    m_process->terminate();
    bool terminated = m_process->waitForFinished(3000);
    if (!terminated) {
        qWarning() << "Process didn't terminate gracefully, killing...";
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_isRunning = false;
    qDebug() << "Process stopped";
}

bool NodeLoader::statuscheck() const
{
    return m_isRunning && (m_process->state() == QProcess::Running);
}

void NodeLoader::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isRunning = false;
    if (exitStatus == QProcess::NormalExit) {
        qDebug() << "Process finished with exit code:" << exitCode;
    } else {
        qDebug() << "Process crashed (exit status:" << exitStatus << ")";
    }
    emit processFinished(exitCode, exitStatus);
}

void NodeLoader::onProcessError(QProcess::ProcessError error)
{
    m_isRunning = false;
    QString errorString = m_process->errorString();
    qCritical() << "Process error occurred:" << error << "-" << errorString;
    emit processError(error, errorString);
}

// 新增：读取并打印进程输出
void NodeLoader::onReadyRead()
{
    // 由于使用了 MergedChannels，标准输出和错误都从这里读取
    QByteArray data = m_process->readAll();
    if (!data.isEmpty()) {
        // 使用 qDebug().noquote() 避免转义，按行输出或整体输出
        // 这里按行分割打印，便于阅读（也可直接整体打印）
        QString output = QString::fromLocal8Bit(data);
        // 去掉末尾换行，避免多余空行
        if (output.endsWith('\n'))
            output.chop(1);
        qDebug().noquote() << "[Process Output]" << output;
    }
}
