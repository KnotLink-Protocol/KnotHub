#include "nodemanager.h"
#include "nodeloader.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QThread>
#include "kludf.h"

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
    , m_pluginsRoot(QDir::currentPath() + "/Plugins")
{
    m_openSocketResponser = new OpenSocketResponser("0x00000002","0x00000011");
    connect(m_openSocketResponser, &OpenSocketResponser::receivedData,
            this, &PluginManager::onKnotLinkRecieveData);
}

void PluginManager::setPluginsRoot(const QString &path)
{
    QDir dir(path);
    if (dir.exists())
        m_pluginsRoot = dir.absolutePath();
    else
        qWarning() << "Plugins root does not exist:" << path;
}

QStringList PluginManager::refreshPluginList()
{
    QDir pluginsDir(m_pluginsRoot);
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory not found:" << m_pluginsRoot;
        return QStringList();
    }

    // 获取所有子文件夹（不递归）
    QFileInfoList subDirs = pluginsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QStringList currentPluginNames;
    QMap<QString, PluginInfo> newInfos;

    for (const QFileInfo &dirInfo : subDirs) {
        QString folderPath = dirInfo.absoluteFilePath();
        QString manifestPath = folderPath + "/plugin_manifest.json";
        QFile file(manifestPath);
        if (!file.exists()) {
            qDebug() << "No manifest in" << folderPath << ", skipping";
            continue;
        }
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open" << manifestPath;
            continue;
        }
        QByteArray data = file.readAll();
        file.close();

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error in" << manifestPath << ":" << error.errorString();
            continue;
        }
        if (!doc.isObject()) {
            qWarning() << "Manifest root is not an object:" << manifestPath;
            continue;
        }

        PluginInfo info = PluginInfo::fromJson(doc.object(), folderPath);
        qDebug() << "Parsed plugin:" << info.pluginName << "exe_path:" << info.exePath << "abs:" << info.absoluteExePath();
        if (!info.isValid()) {
            qWarning() << "Invalid plugin info in" << manifestPath
                       << "- pluginName empty?" << info.pluginName.isEmpty()
                       << "exePath empty?" << info.exePath.isEmpty()
                       << "exe exists?" << QFile::exists(info.absoluteExePath());
            continue;
        }

        if (!info.isValid()) {
            qWarning() << "Invalid plugin info in" << manifestPath << ": missing name or exe";
            continue;
        }
        newInfos[info.pluginName] = info;
        currentPluginNames << info.pluginName;
    }

    // 找出需要移除的插件（已删除的）
    QStringList oldNames = m_pluginInfos.keys();
    for (const QString &name : oldNames) {
        if (!newInfos.contains(name)) {
            // 如果正在运行，先停止
            if (isPluginRunning(name)) {
                stopPlugin(name);
            }
            removeLoader(name);
            m_pluginInfos.remove(name);
            emit pluginStopped(name);
        }
    }

    // 新增或更新的插件
    bool changed = false;
    for (const QString &name : newInfos.keys()) {
        if (!m_pluginInfos.contains(name) || m_pluginInfos[name].absoluteExePath() != newInfos[name].absoluteExePath()) {
            m_pluginInfos[name] = newInfos[name];
            // 确保 loader 存在（但尚未启动）
            getOrCreateLoader(name);
            changed = true;
        }
    }

    if (changed) {
        emit pluginListChanged(m_pluginInfos.keys());
    }

    return currentPluginNames;
}

NodeLoader* PluginManager::getOrCreateLoader(const QString &pluginName)
{
    if (m_pluginLoaders.contains(pluginName))
        return m_pluginLoaders[pluginName];

    NodeLoader *loader = new NodeLoader(this);
    m_pluginLoaders[pluginName] = loader;

    // 连接信号以便转发
    connect(loader, &NodeLoader::processFinished,
            this, [this, pluginName](int exitCode, QProcess::ExitStatus status) {
        onPluginProcessFinished(pluginName, exitCode, status);
    });
    connect(loader, &NodeLoader::processError,
            this, [this, pluginName](QProcess::ProcessError error, const QString &errStr) {
        onPluginProcessError(pluginName, error, errStr);
    });
    return loader;
}

void PluginManager::removeLoader(const QString &pluginName)
{
    if (m_pluginLoaders.contains(pluginName)) {
        NodeLoader *loader = m_pluginLoaders.take(pluginName);
        delete loader;
    }
}

bool PluginManager::startPlugin(const QString &pluginName, const QStringList &args)
{
    if (!m_pluginInfos.contains(pluginName)) {
        emit pluginError(pluginName, "Plugin not found");
        return false;
    }

    NodeLoader *loader = getOrCreateLoader(pluginName);
    if (loader->statuscheck()) {
        qDebug() << "Plugin already running:" << pluginName;
        return false;
    }

    PluginInfo info = m_pluginInfos[pluginName];
    QString exe = info.absoluteExePath();
    loader->start(exe, args);
    if (loader->statuscheck()) {
        emit pluginStarted(pluginName);
        return true;
    } else {
        emit pluginError(pluginName, "Failed to start process");
        return false;
    }
}

bool PluginManager::startPluginByAppId(const QString &appId, const QStringList &args)
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->appId == appId) {
            return startPlugin(it.key(), args);
        }
    }
    emit pluginError(appId, "No plugin with given app_id");
    return false;
}

bool PluginManager::stopPlugin(const QString &pluginName)
{
    if (!m_pluginLoaders.contains(pluginName))
        return true; // 未加载过，视为已停止

    NodeLoader *loader = m_pluginLoaders[pluginName];
    if (!loader->statuscheck())
        return true;

    loader->stop();
    if (!loader->statuscheck()) {
        emit pluginStopped(pluginName);
        return true;
    } else {
        emit pluginError(pluginName, "Failed to stop");
        return false;
    }
}

bool PluginManager::stopPluginByAppId(const QString &appId)
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->appId == appId) {
            return stopPlugin(it.key());
        }
    }
    return false;
}

bool PluginManager::restartPlugin(const QString &pluginName)
{
    if (!stopPlugin(pluginName))
        return false;
    QThread::msleep(100);
    return startPlugin(pluginName);
}

void PluginManager::startAllPlugins()
{
    for (const QString &name : m_pluginInfos.keys()) {
        startPlugin(name);
    }
}

void PluginManager::stopAllPlugins()
{
    for (const QString &name : m_pluginInfos.keys()) {
        stopPlugin(name);
    }
}

void PluginManager::startAutoStartPlugins()
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->autoStart) {
            startPlugin(it.key());
        }
    }
}

bool PluginManager::isPluginRunning(const QString &pluginName) const
{
    if (!m_pluginLoaders.contains(pluginName))
        return false;
    return m_pluginLoaders[pluginName]->statuscheck();
}

PluginInfo PluginManager::pluginInfo(const QString &pluginName) const
{
    return m_pluginInfos.value(pluginName);
}

void PluginManager::onPluginProcessFinished(const QString &pluginName, int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    emit pluginStopped(pluginName);
}

void PluginManager::onPluginProcessError(const QString &pluginName, QProcess::ProcessError error, const QString &errorString)
{
    Q_UNUSED(error);
    emit pluginError(pluginName, errorString);
}

bool PluginManager::updatePluginConfig(const QString &pluginName, const QString &autostart)
{
    if (!m_pluginInfos.contains(pluginName))
        return false;

    PluginInfo &info = m_pluginInfos[pluginName];

    // 将字符串 "true"/"false" 转换为 bool 值
    info.autoStart = (autostart.compare("true", Qt::CaseInsensitive) == 0);

    // 保存到 manifest 文件
    return savePluginManifest(info);
}

QByteArray PluginManager::exportPluginListToJson()
{
    QJsonArray pluginsArray;
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        const PluginInfo &info = it.value();
        QJsonObject obj;
        obj["plugin_name"] = info.pluginName;
        obj["author"] = info.author;
        obj["app_id"] = info.appId;
        obj["version"] = info.version;
        obj["status"] = isPluginRunning(info.pluginName)?"运行中":"停止";
        pluginsArray.append(obj);
    }
    QJsonObject root;
    root["plugins"] = pluginsArray;
    return QJsonDocument(root).toJson();
}

void PluginManager::onKnotLinkRecieveData(const QString &data, QString questionID){
    // 获取插件列表
    KLKVMap kvMap;
    kvMap.deserialize(data);

    qDebug() << "反序列化结果：" << kvMap;

    QString relpy_str = "ok";
    QString cmd = kvMap["cmd"];

    if(cmd == "get_plugin_list")
    {
        QByteArray json = exportPluginListToJson();
        qDebug() << data << json;
        relpy_str = QString::fromUtf8(json);
    }else if(cmd == "get_detail")
    {
        QString plugin_name = kvMap["plugin_name"];
        PluginInfo info = this->pluginInfo(plugin_name);
        if (info.isValid()) {
            QByteArray jsonData = info.toJson(isPluginRunning(plugin_name)?"运行中":"停止");
            relpy_str = QString::fromUtf8(jsonData);
            qDebug() << relpy_str;
        } else {
            // 未找到
        }
    }
    else if (cmd == "plugin_control")
    {
        QString action = kvMap["action"];
        QString plugin_name = kvMap["plugin_name"];
        bool success = false;
        QString errorMsg;

        if (plugin_name == "all")
        {
            if (action == "start")
            {
                startAllPlugins();
                success = true;
            }
            else if (action == "stop")
            {
                stopAllPlugins();
                success = true;
            }
            else if (action == "restart")
            {
                stopAllPlugins();
                startAllPlugins();
                success = true;
            }
            else
            {
                errorMsg = "Unsupported action: " + action + " for all plugins";
            }
        }
        else
        {
            if (action == "start")
            {
                success = startPlugin(plugin_name);
                if (!success) errorMsg = "Failed to start plugin: " + plugin_name;
            }
            else if (action == "stop")
            {
                success = stopPlugin(plugin_name);
                if (!success) errorMsg = "Failed to stop plugin: " + plugin_name;
            }
            else if (action == "restart")
            {
                success = restartPlugin(plugin_name);
                if (!success) errorMsg = "Failed to restart plugin: " + plugin_name;
            }
            else
            {
                errorMsg = "Unsupported action: " + action;
            }
        }

        // 构造响应 JSON
        QJsonObject response;
        if (success && errorMsg.isEmpty())
        {
            response["status"] = "ok";
            response["message"] = QString("Plugin %1 %succeeded").arg(plugin_name).arg(
                        (action == "start" ? "start " : action == "stop" ? "stop " : "restart "));
        }
        else
        {
            response["status"] = "error";
            response["error"] = errorMsg.isEmpty() ? "Unknown error" : errorMsg;
        }
        relpy_str = QString::fromUtf8(QJsonDocument(response).toJson());
    }
    else if(cmd=="update_config")
    {
        QString pluginName = kvMap["plugin_name"];
        QString autostart  = kvMap["autostart"];
        bool success = updatePluginConfig(pluginName, autostart);
        relpy_str=success?"successful":"failed";
    }else if(cmd=="refresh")
    {
        refreshPluginList();
        QByteArray json = exportPluginListToJson();
        relpy_str = QString::fromUtf8(json);
    }


    QString s = "OK";
    //    operationInfo.deserialize(s);
    m_openSocketResponser->sendBack(relpy_str,questionID);
}

bool PluginManager::savePluginManifest(const QString &pluginName)
{
    if (!m_pluginInfos.contains(pluginName))
        return false;
    return savePluginManifest(m_pluginInfos[pluginName]);
}

bool PluginManager::savePluginManifest(const PluginInfo &info)
{
    QString manifestPath = info.folderPath + "/plugin_manifest.json";
    QFile file(manifestPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open manifest for writing:" << manifestPath;
        return false;
    }

    // 将 PluginInfo 转换为 QJsonObject
    QJsonObject obj;
    obj["plugin_name"] = info.pluginName;
    obj["app_id"] = info.appId;
    obj["author"] = info.author;
    obj["description"] = info.description;
    obj["auto_start"] = info.autoStart ? "true" : "false";
    obj["exe_path"] = info.exePath;
    obj["version"] = info.version;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Indented); // 保持可读性

    if (file.write(data) == -1) {
        qWarning() << "Failed to write manifest:" << manifestPath;
        return false;
    }

    file.close();
    qDebug() << "Saved manifest for plugin:" << info.pluginName;
    return true;
}
