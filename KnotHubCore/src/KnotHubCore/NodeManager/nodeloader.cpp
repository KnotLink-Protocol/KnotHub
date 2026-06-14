#include "nodeloader.h"
#include <QDebug>

NodeLoader::NodeLoader(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isRunning(false)
{
    // 连接进程状态信号
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NodeLoader::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &NodeLoader::onProcessError);
}

NodeLoader::~NodeLoader()
{
    stop();  // 确保进程被终止
}

void NodeLoader::start(const QString &program, const QStringList &arguments)
{
    if (m_isRunning) {
        qWarning() << "Process already running, stop it first";
        return;
    }

    // 清空之前的输出/错误缓冲区（可选）
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    // 启动 exe
    m_process->start(program, arguments);
    bool started = m_process->waitForStarted(3000);  // 等待启动，超时3秒
    if (started) {
        m_isRunning = true;
        qDebug() << "Started:" << program << arguments;
    } else {
        qCritical() << "Failed to start process:" << m_process->errorString();
        m_isRunning = false;
    }
}

void NodeLoader::stop()
{
    if (!m_isRunning)
        return;

    m_process->terminate();                 // 礼貌终止
    bool terminated = m_process->waitForFinished(3000); // 等待3秒
    if (!terminated) {
        qWarning() << "Process didn't terminate, killing...";
        m_process->kill();                  // 强制结束
        m_process->waitForFinished(1000);
    }
    m_isRunning = false;
    qDebug() << "Stopped process";
}

bool NodeLoader::statuscheck() const
{
    // 双重确认：QProcess 状态 + 内部标志
    return m_isRunning && (m_process->state() == QProcess::Running);
}

void NodeLoader::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_isRunning = false;
    if (exitStatus == QProcess::NormalExit) {
        qDebug() << "Process finished with code:" << exitCode;
    } else {
        qDebug() << "Process crashed";
    }
}

void NodeLoader::onProcessError(QProcess::ProcessError error)
{
    m_isRunning = false;
    qCritical() << "Process error:" << error << m_process->errorString();
}
