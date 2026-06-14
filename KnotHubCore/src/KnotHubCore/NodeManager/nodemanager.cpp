#include "nodemanager.h"
#include "nodeloader.h"   // 假设之前实现的 NodeLoader 类在同一项目中

#include <QDir>
#include <QFileInfo>
#include <QDebug>

NodeManager::NodeManager(QObject *parent)
    : QObject(parent)
    , m_workingDirectory(QDir::currentPath())
{
    qDebug() << m_workingDirectory;
    refreshNodeList();
    startNode("MsgNotification1");
}

void NodeManager::setWorkingDirectory(const QString &path)
{
    QDir dir(path);
    if (dir.exists()) {
        m_workingDirectory = dir.absolutePath();
    } else {
        qWarning() << "Working directory does not exist:" << path;
    }
}

QString NodeManager::exePath(const QString &nodeName) const
{
    QDir nodesDir(m_workingDirectory + "/Nodes");
    return nodesDir.filePath(nodeName + ".exe");
}

QString NodeManager::nodeNameFromFilePath(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.baseName();   // 返回不带扩展名的文件名
}

QStringList NodeManager::refreshNodeList()
{
    QDir nodesDir(m_workingDirectory + "/Nodes");
    if (!nodesDir.exists()) {
        qWarning() << "Nodes directory not found:" << nodesDir.absolutePath();
        return QStringList();
    }

    // 获取所有 .exe 文件（不递归子目录）
    QStringList filters;
    filters << "*.exe";
    QFileInfoList exeFiles = nodesDir.entryInfoList(filters, QDir::Files);

    QStringList currentNodes;
    for (const QFileInfo &info : exeFiles) {
        QString nodeName = nodeNameFromFilePath(info.filePath());
        currentNodes << nodeName;
    }

    // 找出新增和删除的节点
    QStringList existingNodes = m_nodeLoaders.keys();
    QStringList added = currentNodes;
    QStringList removed = existingNodes;
    for (const QString &name : existingNodes) {
        added.removeAll(name);
    }
    for (const QString &name : currentNodes) {
        removed.removeAll(name);
    }

    // 删除不再存在的节点
    for (const QString &name : removed) {
        NodeLoader *loader = m_nodeLoaders.take(name);
        if (loader) {
            // 如果还在运行，先停止
            if (loader->statuscheck()) {
                loader->stop();
            }
            delete loader;
            emit nodeStopped(name);
        }
    }

    // 添加新节点
    for (const QString &name : added) {
        NodeLoader *loader = new NodeLoader(this);
        // 连接信号，以便转发状态变化（注意：NodeLoader 需要提供对应的信号，或者我们在此手动转发）
        // 为了简单，我们后续在 startNode 中连接，或者直接使用 loader 的 finished/error 信号并转换
        m_nodeLoaders[name] = loader;

        // 连接信号：当 loader 的进程结束时，我们转发 nodeStopped
        connect(loader, &NodeLoader::processFinished,
                this, [this, name](int exitCode, QProcess::ExitStatus status) {
                    Q_UNUSED(exitCode);
                    Q_UNUSED(status);
                    emit nodeStopped(name);
                });
        connect(loader, &NodeLoader::processError,
                this, [this, name](QProcess::ProcessError error, const QString &errorString) {
                    emit nodeError(name, errorString);
                });
    }

    if (!added.isEmpty() || !removed.isEmpty()) {
        emit nodeListChanged(m_nodeLoaders.keys());
    }

    return currentNodes;
}

bool NodeManager::startNode(const QString &nodeName, const QStringList &arguments)
{
    NodeLoader *loader = m_nodeLoaders.value(nodeName);
    if (!loader) {
        qWarning() << "Node not found:" << nodeName;
        emit nodeError(nodeName, "Node not found");
        return false;
    }

    if (loader->statuscheck()) {
        qWarning() << "Node already running:" << nodeName;
        return false;
    }

    QString exe = exePath(nodeName);
    loader->start(exe, arguments);
    // 注意：NodeLoader::start 是同步启动并等待 started 信号，返回后若成功则 m_isRunning 为 true
    if (loader->statuscheck()) {
        emit nodeStarted(nodeName);
        return true;
    } else {
        emit nodeError(nodeName, "Failed to start");
        return false;
    }
}

bool NodeManager::stopNode(const QString &nodeName)
{
    NodeLoader *loader = m_nodeLoaders.value(nodeName);
    if (!loader) {
        qWarning() << "Node not found:" << nodeName;
        return false;
    }

    if (!loader->statuscheck()) {
        // 已经停止
        return true;
    }

    loader->stop();
    // 注意：stop 是同步的，会等待进程结束
    if (!loader->statuscheck()) {
        emit nodeStopped(nodeName);
        return true;
    } else {
        emit nodeError(nodeName, "Failed to stop");
        return false;
    }
}

bool NodeManager::restartNode(const QString &nodeName)
{
    // 简化：先停止，再启动（无参数）
    if (!stopNode(nodeName))
        return false;
    // 等待一下确保完全停止（可选）
    QThread::msleep(100);
    return startNode(nodeName);
}

void NodeManager::startAllNodes()
{
    for (const QString &name : m_nodeLoaders.keys()) {
        startNode(name);
    }
}

void NodeManager::stopAllNodes()
{
    for (const QString &name : m_nodeLoaders.keys()) {
        stopNode(name);
    }
}

void NodeManager::restartAllNodes()
{
    for (const QString &name : m_nodeLoaders.keys()) {
        restartNode(name);
    }
}

bool NodeManager::isNodeRunning(const QString &nodeName) const
{
    NodeLoader *loader = m_nodeLoaders.value(nodeName);
    return loader ? loader->statuscheck() : false;
}

NodeLoader* NodeManager::nodeLoader(const QString &nodeName) const
{
    return m_nodeLoaders.value(nodeName, nullptr);
}

// 以下私有槽函数用于转发 NodeLoader 信号（如果 NodeLoader 有提供的话，我们已经在 refresh 中使用了 lambda，也可以不使用单独的槽）
// 为保持完整性，这里简单实现两个空槽（实际未使用，但保留接口）
void NodeManager::onNodeFinished(const QString &nodeName, int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    emit nodeStopped(nodeName);
}

void NodeManager::onNodeError(const QString &nodeName, QProcess::ProcessError error, const QString &errorString)
{
    Q_UNUSED(error);
    emit nodeError(nodeName, errorString);
}

//NodeManager::NodeManager()
//{
//    NodeLoader loader;
//    qDebug() << "hh";
//    loader.start("E:/ScientificAndTechnological/ApplicationDesign/Project/KnotLink/KnotLinkedFlexiNode/MsgNotification/dist/MsgNotification.exe", QStringList() << "--arg1" << "value");
//    if (loader.statuscheck())
//        qDebug() << "Running";
//    loader.stop();
//}
