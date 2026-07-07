#include "nodemanager.h"
#include "nodeloader.h"
#include <QDir>
#include <QFile>
#include <QDirIterator>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QThread>
#include "../quazip/JlCompress.h"
#include "kludf.h"

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
    , m_pluginsRoot(QDir::currentPath() + "/Plugins")
{
    m_openSocketResponser = new OpenSocketResponser("0x00000002", "0x00000011");
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

// ═══════════════════════════════════════════════════════════════
// 插件列表刷新
// ═══════════════════════════════════════════════════════════════

QStringList PluginManager::refreshPluginList()
{
    QDir pluginsDir(m_pluginsRoot);
    if (!pluginsDir.exists()) {
        qWarning() << "Plugins directory not found:" << m_pluginsRoot;
        return QStringList();
    }

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
        if (!info.isValid()) {
            qWarning() << "Invalid plugin info in" << manifestPath;
            continue;
        }
        newInfos[info.pluginName] = info;
        currentPluginNames << info.pluginName;
    }

    // 移除已删除的插件
    QStringList oldNames = m_pluginInfos.keys();
    for (const QString &name : oldNames) {
        if (!newInfos.contains(name)) {
            if (isPluginRunning(name))
                stopPlugin(name);
            removeLoader(name);
            m_pluginInfos.remove(name);
            emit pluginStopped(name);
        }
    }

    // 新增或更新
    bool changed = false;
    for (const QString &name : newInfos.keys()) {
        if (!m_pluginInfos.contains(name) ||
            m_pluginInfos[name].absoluteExePath() != newInfos[name].absoluteExePath()) {
            m_pluginInfos[name] = newInfos[name];
            getOrCreateLoader(name);
            changed = true;
        }
    }

    if (changed)
        emit pluginListChanged(m_pluginInfos.keys());

    return currentPluginNames;
}

// ═══════════════════════════════════════════════════════════════
// Loader 管理
// ═══════════════════════════════════════════════════════════════

NodeLoader* PluginManager::getOrCreateLoader(const QString &pluginName)
{
    if (m_pluginLoaders.contains(pluginName))
        return m_pluginLoaders[pluginName];

    NodeLoader *loader = new NodeLoader(this);
    m_pluginLoaders[pluginName] = loader;

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
    if (m_pluginLoaders.contains(pluginName))
        delete m_pluginLoaders.take(pluginName);
}

// ═══════════════════════════════════════════════════════════════
// 启停
// ═══════════════════════════════════════════════════════════════

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
    loader->start(info.absoluteExePath(), args);
    if (loader->statuscheck()) {
        emit pluginStarted(pluginName);
        return true;
    }
    emit pluginError(pluginName, "Failed to start process");
    return false;
}

bool PluginManager::startPluginByAppId(const QString &appId, const QStringList &args)
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->appId == appId)
            return startPlugin(it.key(), args);
    }
    emit pluginError(appId, "No plugin with given app_id");
    return false;
}

bool PluginManager::stopPlugin(const QString &pluginName)
{
    if (!m_pluginLoaders.contains(pluginName))
        return true;

    NodeLoader *loader = m_pluginLoaders[pluginName];
    if (!loader->statuscheck())
        return true;

    loader->stop();
    if (!loader->statuscheck()) {
        emit pluginStopped(pluginName);
        return true;
    }
    emit pluginError(pluginName, "Failed to stop");
    return false;
}

bool PluginManager::stopPluginByAppId(const QString &appId)
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->appId == appId)
            return stopPlugin(it.key());
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
    for (const QString &name : m_pluginInfos.keys())
        startPlugin(name);
}

void PluginManager::stopAllPlugins()
{
    for (const QString &name : m_pluginInfos.keys())
        stopPlugin(name);
}

void PluginManager::startAutoStartPlugins()
{
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        if (it->autoStart)
            startPlugin(it.key());
    }
}

bool PluginManager::isPluginRunning(const QString &pluginName) const
{
    return m_pluginLoaders.contains(pluginName) &&
           m_pluginLoaders[pluginName]->statuscheck();
}

PluginInfo PluginManager::pluginInfo(const QString &pluginName) const
{
    return m_pluginInfos.value(pluginName);
}

// ═══════════════════════════════════════════════════════════════
// 进程信号
// ═══════════════════════════════════════════════════════════════

void PluginManager::onPluginProcessFinished(const QString &pluginName,
    int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode); Q_UNUSED(exitStatus);
    emit pluginStopped(pluginName);
}

void PluginManager::onPluginProcessError(const QString &pluginName,
    QProcess::ProcessError error, const QString &errorString)
{
    Q_UNUSED(error);
    emit pluginError(pluginName, errorString);
}

// ═══════════════════════════════════════════════════════════════
// 导出
// ═══════════════════════════════════════════════════════════════

QByteArray PluginManager::exportPluginListToJson()
{
    QJsonArray pluginsArray;
    for (auto it = m_pluginInfos.begin(); it != m_pluginInfos.end(); ++it) {
        const PluginInfo &info = it.value();
        QJsonObject obj;
        obj["plugin_name"] = info.pluginName;
        obj["author"]      = info.author;
        obj["app_id"]      = info.appId;
        obj["version"]     = info.version;
        obj["status"]      = isPluginRunning(info.pluginName) ? "运行中" : "停止";
        obj["node_type"]   = "plugin";
        obj["description"] = info.description;
        obj["auto_start"]  = info.autoStart ? "true" : "false";
        pluginsArray.append(obj);
    }
    QJsonObject root;
    root["plugins"] = pluginsArray;
    return QJsonDocument(root).toJson();
}

// ═══════════════════════════════════════════════════════════════
// 配置
// ═══════════════════════════════════════════════════════════════

bool PluginManager::updatePluginConfig(const QString &pluginName, const QString &autostart)
{
    if (!m_pluginInfos.contains(pluginName))
        return false;

    PluginInfo &info = m_pluginInfos[pluginName];
    info.autoStart = (autostart.compare("true", Qt::CaseInsensitive) == 0);
    return savePluginManifest(info);
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
        qWarning() << "Cannot write manifest:" << manifestPath;
        return false;
    }

    QJsonObject obj;
    obj["plugin_name"] = info.pluginName;
    obj["app_id"]      = info.appId;
    obj["author"]      = info.author;
    obj["description"] = info.description;
    obj["auto_start"]  = info.autoStart ? "true" : "false";
    obj["exe_path"]    = info.exePath;
    obj["version"]     = info.version;

    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

// ═══════════════════════════════════════════════════════════════
// 安装插件 — 解压 zip 到 Plugins/ 目录
// ═══════════════════════════════════════════════════════════════

bool PluginManager::installPlugin(const QString &zipPath, QString &error)
{
    // 1. 检查 zip 文件
    if (!QFile::exists(zipPath)) {
        error = QString("Zip file not found: %1").arg(zipPath);
        return false;
    }

    // 2. 先解压到临时目录，读取 manifest 获取 plugin_name
    QTemporaryDir tmpDir;
    if (!tmpDir.isValid()) {
        error = "Failed to create temp directory";
        return false;
    }

    QStringList extracted = JlCompress::extractDir(zipPath, tmpDir.path());
    if (extracted.isEmpty()) {
        error = "Failed to extract zip (corrupted or empty)";
        return false;
    }

    // 查找 plugin_manifest.json（可能在子目录里）
    QString manifestPath;
    QDirIterator it(tmpDir.path(), {"plugin_manifest.json"}, QDir::Files,
                    QDirIterator::Subdirectories);
    if (it.hasNext()) {
        manifestPath = it.next();
    }

    QString pluginName;
    if (!manifestPath.isEmpty()) {
        QFile f(manifestPath);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            if (doc.isObject()) {
                pluginName = doc.object().value("plugin_name").toString();
                if (pluginName.isEmpty())
                    pluginName = doc.object().value("app_id").toString();
            }
        }
    }

    if (pluginName.isEmpty()) {
        // fallback：用 zip 文件名（去掉扩展名）
        QFileInfo fi(zipPath);
        pluginName = fi.completeBaseName();
    }

    // 3. 解压到 Plugins/<plugin_name>/
    QString destDir = m_pluginsRoot + "/" + pluginName;
    QDir().mkpath(destDir);

    // 把临时目录里所有文件复制到目标目录
    QDir tmpDirRoot(tmpDir.path());
    QStringList files = tmpDirRoot.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &f : files) {
        QString src  = tmpDir.path() + "/" + f;
        QString dest = destDir + "/" + f;
        if (QFileInfo(src).isDir()) {
            copyDirRecursive(src, dest);
        } else {
            QFile::copy(src, dest);
        }
    }

    // 4. 验证：目标目录必须有 plugin_manifest.json
    QString finalManifest = destDir + "/plugin_manifest.json";
    if (!QFile::exists(finalManifest)) {
        error = "Installed but no plugin_manifest.json found — "
                "plugin may not be recognized";
        return false;
    }

    // 5. 刷新列表
    refreshPluginList();

    qDebug() << "[Plugin] Installed:" << pluginName << "from" << zipPath;
    return true;
}

void PluginManager::copyDirRecursive(const QString &src, const QString &dst)
{
    QDir().mkpath(dst);
    QDir srcDir(src);
    QStringList entries = srcDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        QString s = src  + "/" + entry;
        QString d = dst  + "/" + entry;
        if (QFileInfo(s).isDir()) {
            copyDirRecursive(s, d);
        } else {
            QFile::copy(s, d);
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// KnotLink 消息处理 — socketID: 0x00000011（仅插入式节点）
// ═══════════════════════════════════════════════════════════════

void PluginManager::onKnotLinkRecieveData(const QString &data, QString questionID)
{
    KLKVMap kvMap;
    kvMap.deserialize(data);

    qDebug() << "[Plugin] KL data:" << kvMap;

    QString cmd = kvMap["cmd"];
    QString reply = "ok";

    if (cmd == "get_plugin_list") {
        reply = QString::fromUtf8(exportPluginListToJson());
        qDebug() << reply;

    } else if (cmd == "get_detail") {
        QString plugin_name = kvMap["plugin_name"];
        PluginInfo info = pluginInfo(plugin_name);
        if (info.isValid()) {
            reply = QString::fromUtf8(
                info.toJson(isPluginRunning(plugin_name) ? "运行中" : "停止"));
        } else {
            reply = "Error: plugin not found";
        }

    } else if (cmd == "plugin_control") {
        QString action      = kvMap["action"];
        QString plugin_name = kvMap["plugin_name"];
        bool    success     = false;
        QString errorMsg;

        if (plugin_name == "all") {
            if (action == "start") {
                startAllPlugins(); success = true;
            } else if (action == "stop") {
                stopAllPlugins();  success = true;
            } else if (action == "restart") {
                stopAllPlugins();  startAllPlugins(); success = true;
            } else {
                errorMsg = "Unsupported action: " + action;
            }
        } else {
            if (action == "start") {
                success = startPlugin(plugin_name);
            } else if (action == "stop") {
                success = stopPlugin(plugin_name);
            } else if (action == "restart") {
                success = restartPlugin(plugin_name);
            } else {
                errorMsg = "Unsupported action: " + action;
            }
            if (!success && errorMsg.isEmpty())
                errorMsg = "Failed to " + action + " plugin: " + plugin_name;
        }
        reply = success ? "ok" : ("error: " + errorMsg);

    } else if (cmd == "update_config") {
        QString pluginName = kvMap["plugin_name"];
        QString autostart  = kvMap["autostart"];
        reply = updatePluginConfig(pluginName, autostart) ? "successful" : "failed";

    } else if (cmd == "get_funclist") {
        QString plugin_name = kvMap["plugin_name"];
        if (plugin_name.isEmpty()) {
            reply = "Error: missing plugin_name";
        } else {
            PluginInfo info = pluginInfo(plugin_name);
            if (!info.isValid()) {
                reply = "Error: plugin not found";
            } else {
                QString path = info.folderPath + "/FuncList.json";
                QFile file(path);
                if (!file.exists()) {
                    reply = "Error: FuncList.json not found";
                } else if (!file.open(QIODevice::ReadOnly)) {
                    reply = "Error: cannot open FuncList.json";
                } else {
                    reply = QString::fromUtf8(file.readAll());
                    file.close();
                }
            }
        }

    } else if (cmd == "install_plugin") {
        QString zipPath = kvMap["zip_path"];
        QString err;
        bool ok = installPlugin(zipPath, err);
        reply = ok ? "ok" : ("error: " + err);

    } else if (cmd == "refresh") {
        refreshPluginList();
        reply = QString::fromUtf8(exportPluginListToJson());

    } else {
        reply = "Error: unknown command: " + cmd;
    }

    m_openSocketResponser->sendBack(reply, questionID);
}
