#include "nodeloader.h"
#include <QDebug>

NodeLoader::NodeLoader(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isRunning(false)
{
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &NodeLoader::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &NodeLoader::onProcessError);
}

NodeLoader::~NodeLoader()
{
    qWarning() << "Process already running, stop it first";
    stop();
}

void NodeLoader::start(const QString &program, const QStringList &arguments)
{
    if (m_isRunning) {
        qWarning() << "Prt";
        return;
    }

    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start(program, arguments);
    bool started = m_process->waitForStarted(3000);
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

    m_process->terminate();
    bool terminated = m_process->waitForFinished(3000);
    if (!terminated) {
        qWarning() << "Process didn't terminate, killing...";
        m_process->kill();
        m_process->waitForFinished(1000);
    }
    m_isRunning = false;
    qDebug() << "Stopped process";
}

bool NodeLoader::statuscheck() const
{
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
    emit processFinished(exitCode, exitStatus);
}

void NodeLoader::onProcessError(QProcess::ProcessError error)
{
    m_isRunning = false;
    QString errorString = m_process->errorString();
    qCritical() << "Process error:" << error << errorString;
    emit processError(error, errorString);
}
