#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include <KnotLinkLib>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <QFile>
#include <QDir>


#include "plugininfo.h"

class NodeLoader;
class RecipeManager;

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);

    // 设置插件根目录（默认为应用程序所在目录下的 "Plugins"）
    void setPluginsRoot(const QString &path);
    QString pluginsRoot() const { return m_pluginsRoot; }

    // 扫描根目录下的所有子文件夹，读取 manifest，更新插件列表
    // 返回新增的插件名列表
    QStringList refreshPluginList();

    // 根据插件名启动插件
    bool startPlugin(const QString &pluginName, const QStringList &args = QStringList());
    // 根据 appId 启动插件
    bool startPluginByAppId(const QString &appId, const QStringList &args = QStringList());

    bool stopPlugin(const QString &pluginName);
    bool stopPluginByAppId(const QString &appId);

    bool restartPlugin(const QString &pluginName);

    void startAllPlugins();
    void stopAllPlugins();
    void startAutoStartPlugins();   // 启动所有 auto_start 为 true 的插件

    QStringList pluginNames() const { return m_pluginInfos.keys(); }
    PluginInfo pluginInfo(const QString &pluginName) const;
    bool isPluginRunning(const QString &pluginName) const;

    QByteArray exportPluginListToJson();

    // 独立式节点（注册表发现）
    struct StandaloneNode {
        QString appId;
        QString installPath;
        QString name;
    };
    QList<StandaloneNode> standaloneNodes() const { return m_standaloneNodes; }
    void scanStandaloneNodes();

    // 配方命令转发
    void setRecipeManager(RecipeManager *rm) { m_recipeManager = rm; }

signals:
    void pluginListChanged(const QStringList &currentPlugins);
    void pluginStarted(const QString &pluginName);
    void pluginStopped(const QString &pluginName);
    void pluginError(const QString &pluginName, const QString &errorString);

private slots:
    void onPluginProcessFinished(const QString &pluginName, int exitCode, QProcess::ExitStatus exitStatus);
    void onPluginProcessError(const QString &pluginName, QProcess::ProcessError error, const QString &errorString);

    // KnotLink支持
    void onKnotLinkRecieveData(const QString &data,QString questionID);

private:
    QString m_pluginsRoot;
    QMap<QString, PluginInfo> m_pluginInfos;        // pluginName -> PluginInfo
    QMap<QString, NodeLoader*> m_pluginLoaders;     // pluginName -> NodeLoader*

    // 内部辅助
    NodeLoader* getOrCreateLoader(const QString &pluginName);
    void removeLoader(const QString &pluginName);

    // KnotLink支持
    OpenSocketResponser *m_openSocketResponser;
    KLKVMap operationInfo;
    RecipeManager *m_recipeManager = nullptr;
    QList<StandaloneNode> m_standaloneNodes;

    bool updatePluginConfig(const QString &pluginName, const QString &autostart);
    bool savePluginManifest(const QString &pluginName);
    bool savePluginManifest(const PluginInfo &info);

};

#endif // PLUGINMANAGER_H
