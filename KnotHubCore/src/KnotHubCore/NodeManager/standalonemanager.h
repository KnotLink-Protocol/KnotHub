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

class OpenSocketResponser;
class KLKVMap;

// ── 独立式节点管理器 ──────────────────────────────────────────
// 职责：扫描注册表 → manifest 解析 → 列表导出
// 不管理进程生命周期（独立式应用自行启停）
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

    // ── 导出 ────────────────────────────────────────────────
    QByteArray exportListToJson() const;
    QByteArray getFuncList(const QString &appId) const;

signals:
    void nodeListChanged();
    void logMessage(const QString &message);

private slots:
    void onKnotLinkData(const QString &data, const QString &questionID);

private:
    // ── manifest 解析 ───────────────────────────────────────
    NodeInfo parseManifest(const QString &manifestPath, const QString &installPath);
    void fallbackFromFuncList(const QDir &dir, NodeInfo &info);

    // ── 成员 ────────────────────────────────────────────────
    OpenSocketResponser *m_responder;
    QList<NodeInfo>       m_nodes;
};

#endif // STANDALONEMANAGER_H
