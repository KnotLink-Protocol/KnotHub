#include "nodeloader.h"
#include <QDebug>

// 静态成员 — 全局唯一的 Job Object 句柄
HANDLE NodeLoader::s_jobHandle = NULL;

// 确保 Job Object 存在（所有 NodeLoader 共享同一个 Job）
void NodeLoader::ensureJobObject()
{
    if (s_jobHandle != NULL)
        return;

    // 1. 尝试打开同名 Job（上次崩溃残留），关闭它以清理旧孤儿进程
    HANDLE hExisting = OpenJobObject(JOB_OBJECT_ASSIGN_PROCESS, FALSE,
                                     L"KnotHubCore_Job");
    if (hExisting != NULL) {
        qDebug() << "[NodeLoader] Stale Job Object found from previous crash — cleaning up";
        // TerminateJobObject 会杀死旧 Job 里所有残留进程
        TerminateJobObject(hExisting, 0);
        CloseHandle(hExisting);
    }

    // 2. 创建新 Job
    s_jobHandle = CreateJobObject(NULL, L"KnotHubCore_Job");
    if (s_jobHandle == NULL) {
        qCritical() << "[NodeLoader] CreateJobObject failed:" << GetLastError();
        return;
    }

    // 3. 设置限制：Core 退出时 OS 自动杀死 Job 内所有子进程
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
    jeli.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE   // ← 核心：句柄关闭 → 杀全 Job
        | JOB_OBJECT_LIMIT_BREAKAWAY_OK;     // 允许子进程的子进程脱离 Job

    if (!SetInformationJobObject(s_jobHandle,
            JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
        qCritical() << "[NodeLoader] SetInformationJobObject failed:" << GetLastError();
    }

    qDebug() << "[NodeLoader] Job Object created — child processes will be auto-killed on Core exit";
}

NodeLoader::NodeLoader(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_isRunning(false)
{
    // 确保 Job Object 存在（只首次创建）
    ensureJobObject();
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

        // 将子进程加入 Job Object，确保 Core 强制退出时 OS 自动清理
        if (s_jobHandle != NULL) {
            qint64 pid = m_process->processId();
            if (pid > 0) {
                HANDLE hProcess = OpenProcess(
                    PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, (DWORD)pid);
                if (hProcess != NULL) {
                    if (!AssignProcessToJobObject(s_jobHandle, hProcess)) {
                        qWarning() << "[NodeLoader] AssignProcessToJobObject failed:"
                                   << GetLastError() << "pid:" << pid;
                    }
                    CloseHandle(hProcess);
                }
            }
        }
    } else {
        qCritical() << "Failed to start process:" << m_process->errorString();
        m_isRunning = false;
    }
}

void NodeLoader::stop()
{
    if (!m_isRunning)
        return;

    qint64 pid = m_process->processId();

    // 1. taskkill /F /T 先杀进程树（含所有子进程）
    if (pid > 0) {
        QProcess::execute("taskkill", QStringList()
            << "/F" << "/T" << "/PID" << QString::number(pid));
        m_process->waitForFinished(2000);
    }

    // 2. 如果 taskkill 失败，QProcess 兜底
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        m_process->waitForFinished(2000);
    }
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }

    m_isRunning = false;
    qDebug() << "Process stopped (pid:" << pid << ")";
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
