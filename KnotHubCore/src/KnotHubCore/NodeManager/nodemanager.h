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

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);

    // 设置插件根目录（默认为应用程序所在目录下的 "Plugins"）
    void setPluginsRoot(const QString &path);
    QString pluginsRoot() const { return m_pluginsRoot; }

    // 扫描根目录下的所有子文件夹，读取 manifest，更新插件列表
    QStringList refreshPluginList();

    // 启停
    bool startPlugin(const QString &pluginName, const QStringList &args = QStringList());
    bool startPluginByAppId(const QString &appId, const QStringList &args = QStringList());
    bool stopPlugin(const QString &pluginName);
    bool stopPluginByAppId(const QString &appId);
    bool restartPlugin(const QString &pluginName);

    void startAllPlugins();
    void stopAllPlugins();
    void startAutoStartPlugins();

    // 查询
    QStringList pluginNames() const { return m_pluginInfos.keys(); }
    PluginInfo pluginInfo(const QString &pluginName) const;
    bool isPluginRunning(const QString &pluginName) const;

    // 导出
    QByteArray exportPluginListToJson();

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
    QMap<QString, PluginInfo> m_pluginInfos;
    QMap<QString, NodeLoader*> m_pluginLoaders;

    NodeLoader* getOrCreateLoader(const QString &pluginName);
    void removeLoader(const QString &pluginName);

    OpenSocketResponser *m_openSocketResponser;
    KLKVMap operationInfo;

    bool updatePluginConfig(const QString &pluginName, const QString &autostart);
    bool savePluginManifest(const QString &pluginName);
    bool savePluginManifest(const PluginInfo &info);

    // 安装/删除插件
    bool installPlugin(const QString &zipPath, QString &error);
    bool deletePlugin(const QString &pluginName, QString &error);
    static void copyDirRecursive(const QString &src, const QString &dst);
};

#endif // PLUGINMANAGER_H
