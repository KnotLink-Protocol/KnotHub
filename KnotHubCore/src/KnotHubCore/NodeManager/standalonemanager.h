#ifndef STANDALONEMANAGER_H
#define STANDALONEMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QFileInfo>

class NodeLoader;
class OpenSocketResponser;
class KLKVMap;

// ── 独立式节点管理器 ──────────────────────────────────────────
// 负责：注册表扫描 → manifest 解析 → 探活 → 启停 → 功能清单
// OpenSocket: appID=0x00000002, openSocketID=0x00000012

class StandaloneManager : public QObject
{
    Q_OBJECT

public:
    // ── 独立式节点信息 ──────────────────────────────────────
    struct NodeInfo {
        QString appId;
        QString appName;
        QString description;
        QString version;
        QString author;
        QString installPath;
        QString exePath;
        bool    autoStart = false;
        bool    isOnline  = false;

        QString absoluteExePath() const {
            if (exePath.isEmpty()) return QString();
            return QDir(installPath).filePath(exePath);
        }
        bool isValid() const {
            return !appId.isEmpty() && !installPath.isEmpty()
                   && QDir(installPath).exists();
        }
    };

    explicit StandaloneManager(QObject *parent = nullptr);
    ~StandaloneManager();

    // ── 扫描 ────────────────────────────────────────────────
    void scan();

    // ── 查询 ────────────────────────────────────────────────
    QList<NodeInfo> nodes() const { return m_nodes; }
    NodeInfo nodeInfo(const QString &appId) const;

    // ── 启停 ────────────────────────────────────────────────
    bool startNode(const QString &appId);
    bool stopNode(const QString &appId);
    bool isNodeRunning(const QString &appId) const;
    void startAutoStartNodes();

    // ── 导出 ────────────────────────────────────────────────
    QByteArray exportListToJson() const;
    QByteArray getFuncList(const QString &appId) const;

signals:
    void nodeListChanged();
    void nodeStarted(const QString &appId);
    void nodeStopped(const QString &appId);
    void nodeError(const QString &appId, const QString &errorString);
    void logMessage(const QString &message);

private slots:
    void onKnotLinkData(const QString &data, const QString &questionID);

private:
    // ── manifest 解析 ───────────────────────────────────────
    NodeInfo parseManifest(const QString &manifestPath, const QString &installPath);
    void fallbackFromFuncList(const QDir &dir, NodeInfo &info);
    bool checkAlive(const QString &appId);

    // ── Loader 管理 ─────────────────────────────────────────
    NodeLoader* getOrCreateLoader(const QString &appId);
    void removeLoader(const QString &appId);

    // ── 成员 ────────────────────────────────────────────────
    OpenSocketResponser *m_responder;
    QList<NodeInfo>       m_nodes;
    QMap<QString, NodeLoader*> m_loaders;
};

#endif // STANDALONEMANAGER_H
