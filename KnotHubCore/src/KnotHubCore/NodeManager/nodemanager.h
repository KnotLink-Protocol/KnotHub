#ifndef NODEMANAGER_H
#define NODEMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QProcess>

class NodeLoader;

class NodeManager : public QObject
{
    Q_OBJECT
public:
    explicit NodeManager(QObject *parent = nullptr);

    // 设置工作目录（默认使用应用程序当前目录）
    void setWorkingDirectory(const QString &path);
    QString workingDirectory() const { return m_workingDirectory; }

    // 刷新节点列表：扫描 Nodes 文件夹下的所有 .exe 文件
    // 返回新增的节点名列表
    QStringList refreshNodeList();

    // 启动特定节点（节点名为 exe 文件名，不含路径）
    bool startNode(const QString &nodeName, const QStringList &arguments = QStringList());
    // 停止特定节点
    bool stopNode(const QString &nodeName);
    // 重启特定节点（先 stop 再 start，使用相同参数？需要保存参数，简化版：stop 后再 start 无参数）
    bool restartNode(const QString &nodeName);

    // 启动所有节点
    void startAllNodes();
    // 停止所有节点
    void stopAllNodes();
    // 重启所有节点
    void restartAllNodes();

    // 获取所有节点名称列表（基于当前扫描结果）
    QStringList nodeNames() const { return m_nodeLoaders.keys(); }

    // 查询节点是否正在运行
    bool isNodeRunning(const QString &nodeName) const;

    // 获取节点对应的 NodeLoader 指针（可为 nullptr）
    NodeLoader* nodeLoader(const QString &nodeName) const;

signals:
    // 当节点列表发生变化时（刷新后新增或移除）发出
    void nodeListChanged(const QStringList &currentNodes);

    // 节点启动/停止状态变化信号
    void nodeStarted(const QString &nodeName);
    void nodeStopped(const QString &nodeName);
    void nodeError(const QString &nodeName, const QString &errorString);

private slots:
    void onNodeFinished(const QString &nodeName, int exitCode, QProcess::ExitStatus exitStatus);
    void onNodeError(const QString &nodeName, QProcess::ProcessError error, const QString &errorString);

private:
    // 构建 exe 的完整路径
    QString exePath(const QString &nodeName) const;
    // 从文件路径提取节点名（去掉路径和 .exe 后缀）
    static QString nodeNameFromFilePath(const QString &filePath);

    QString m_workingDirectory;                 // 工作目录
    QMap<QString, NodeLoader*> m_nodeLoaders;   // 节点名 -> NodeLoader 指针（生命周期由 NodeManager 管理）
};

#endif // NODEMANAGER_H
