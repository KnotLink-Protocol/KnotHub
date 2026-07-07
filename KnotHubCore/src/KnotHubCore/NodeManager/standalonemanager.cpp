#include "standalonemanager.h"
#include "nodeloader.h"
#include <KnotLinkLib>

#include <QDebug>
#include <QThread>
#include <QJsonParseError>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// ═══════════════════════════════════════════════════════════════
// 构造 / 析构
// ═══════════════════════════════════════════════════════════════

StandaloneManager::StandaloneManager(QObject *parent)
    : QObject(parent)
{
    m_responder = new OpenSocketResponser("0x00000002", "0x00000012", this);
    connect(m_responder, &OpenSocketResponser::receivedData,
            this, &StandaloneManager::onKnotLinkData);
}

StandaloneManager::~StandaloneManager()
{
    for (auto it = m_loaders.begin(); it != m_loaders.end(); ++it) {
        if (it.value()->statuscheck())
            it.value()->stop();
        delete it.value();
    }
    m_loaders.clear();
}

// ═══════════════════════════════════════════════════════════════
// 扫描 — 对标 PluginManager::refreshPluginList()
// ═══════════════════════════════════════════════════════════════

void StandaloneManager::scan()
{
    m_nodes.clear();

#ifdef Q_OS_WIN
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\KnotLink\\StandaloneNodes",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        qDebug() << "[Standalone] No nodes registered (registry key not found)";
        return;
    }

    DWORD index = 0;
    WCHAR valueName[256];
    WCHAR valueData[MAX_PATH];
    DWORD nameSize, dataSize, type;

    while (true) {
        nameSize = 256;
        dataSize = sizeof(valueData);
        LONG result = RegEnumValueW(hKey, index, valueName, &nameSize,
                                    nullptr, &type, (LPBYTE)valueData, &dataSize);
        if (result == ERROR_NO_MORE_ITEMS) break;
        if (result != ERROR_SUCCESS) break;

        QString appId       = QString::fromWCharArray(valueName);
        QString installPath = QString::fromWCharArray(valueData);

        qDebug() << "[Standalone] Registry entry:" << appId << "->" << installPath;

        QDir dir(installPath);
        if (!dir.exists()) {
            qWarning() << "[Standalone] Path not found, skipping:" << installPath;
            index++;
            continue;
        }

        NodeInfo info;
        info.appId       = appId;
        info.installPath = installPath;

        QString manifestPath = dir.absoluteFilePath("standalone_manifest.json");
        if (QFile::exists(manifestPath)) {
            info = parseManifest(manifestPath, installPath);
            if (info.appId.isEmpty())
                info.appId = appId;
        } else {
            qDebug() << "[Standalone] No standalone_manifest.json, fallback";
            fallbackFromFuncList(dir, info);
        }

        if (!info.isValid()) {
            qWarning() << "[Standalone] Invalid node, skipping:" << installPath;
            index++;
            continue;
        }

        info.isOnline = checkAlive(appId);
        m_nodes.append(info);

        qDebug() << "[Standalone] Node added:" << info.appId << info.appName
                 << "online:" << info.isOnline << "autostart:" << info.autoStart;
        index++;
    }

    RegCloseKey(hKey);
#endif

    emit nodeListChanged();
}

// ═══════════════════════════════════════════════════════════════
// manifest 解析
// ═══════════════════════════════════════════════════════════════

StandaloneManager::NodeInfo StandaloneManager::parseManifest(
    const QString &manifestPath, const QString &installPath)
{
    NodeInfo info;
    info.installPath = installPath;

    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[Standalone] Cannot open:" << manifestPath;
        return info;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Standalone] Bad JSON:" << manifestPath;
        return info;
    }

    QJsonObject obj = doc.object();
    info.appId       = obj.value("app_id").toString();
    info.appName     = obj.value("app_name").toString();
    info.author      = obj.value("author").toString();
    info.version     = obj.value("version").toString();
    info.description = obj.value("description").toString();
    info.autoStart   = obj.value("auto_start").toString()
                           .compare("true", Qt::CaseInsensitive) == 0;
    info.exePath     = obj.value("exe_path").toString();

    return info;
}

void StandaloneManager::fallbackFromFuncList(const QDir &dir, NodeInfo &info)
{
    QString funcListPath = dir.absoluteFilePath("FuncList.json");
    QFile file(funcListPath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isObject())
            info.appName = doc.object().value("appName").toString();
    }

    // 兼容旧 plugin_manifest.json
    QString oldPath = dir.absoluteFilePath("plugin_manifest.json");
    QFile oldFile(oldPath);
    if (oldFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(oldFile.readAll());
        oldFile.close();
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (info.appId.isEmpty())
                info.appId   = obj.value("app_id").toString();
            info.author      = obj.value("author").toString();
            info.version     = obj.value("version").toString();
            info.description = obj.value("description").toString();
            info.autoStart   = obj.value("auto_start").toString()
                                   .compare("true", Qt::CaseInsensitive) == 0;
            info.exePath     = obj.value("exe_path").toString();
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// 探活
// ═══════════════════════════════════════════════════════════════

bool StandaloneManager::checkAlive(const QString &appId)
{
    if (m_loaders.contains(appId))
        return m_loaders[appId]->statuscheck();

    for (const auto &info : m_nodes) {
        if (info.appId != appId) continue;
        if (info.exePath.isEmpty()) return false;

        QString exeName = QFileInfo(info.absoluteExePath()).fileName();
        if (exeName.isEmpty()) return false;

        QProcess proc;
        proc.start("tasklist", QStringList()
                   << "/FI" << QString("IMAGENAME eq %1").arg(exeName)
                   << "/NH" << "/FO" << "CSV");
        proc.waitForFinished(3000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        return output.contains(exeName, Qt::CaseInsensitive);
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════
// 查询
// ═══════════════════════════════════════════════════════════════

StandaloneManager::NodeInfo StandaloneManager::nodeInfo(const QString &appId) const
{
    for (const auto &n : m_nodes) {
        if (n.appId == appId) return n;
    }
    return NodeInfo();
}

// ═══════════════════════════════════════════════════════════════
// 导出
// ═══════════════════════════════════════════════════════════════

QByteArray StandaloneManager::exportListToJson() const
{
    QJsonArray arr;
    for (const auto &n : m_nodes) {
        QJsonObject obj;
        obj["plugin_name"] = n.appId;
        obj["app_id"]      = n.appId;
        obj["author"]      = n.author;
        obj["version"]     = n.version;
        obj["description"] = n.description;
        obj["status"]      = n.isOnline ? "运行中" : "停止";
        obj["node_type"]   = "standalone";
        obj["name"]        = n.appName;
        obj["auto_start"]  = n.autoStart ? "true" : "false";
        arr.append(obj);
    }
    QJsonObject root;
    root["standalone_nodes"] = arr;
    return QJsonDocument(root).toJson();
}

QByteArray StandaloneManager::getFuncList(const QString &appId) const
{
    for (const auto &n : m_nodes) {
        if (n.appId != appId) continue;
        QString path = n.installPath + "/FuncList.json";
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return QByteArray();
        QByteArray content = file.readAll();
        file.close();
        return content;
    }
    return QByteArray();
}

// ═══════════════════════════════════════════════════════════════
// Loader 管理
// ═══════════════════════════════════════════════════════════════

NodeLoader* StandaloneManager::getOrCreateLoader(const QString &appId)
{
    if (m_loaders.contains(appId))
        return m_loaders[appId];

    NodeLoader *loader = new NodeLoader(this);
    m_loaders[appId] = loader;

    connect(loader, &NodeLoader::processFinished,
            this, [this, appId](int, QProcess::ExitStatus) {
        for (int i = 0; i < m_nodes.size(); i++) {
            if (m_nodes[i].appId == appId) {
                m_nodes[i].isOnline = false;
                break;
            }
        }
        emit nodeStopped(appId);
    });
    connect(loader, &NodeLoader::processError,
            this, [this, appId](QProcess::ProcessError, const QString &err) {
        emit nodeError(appId, err);
    });

    return loader;
}

void StandaloneManager::removeLoader(const QString &appId)
{
    if (m_loaders.contains(appId)) {
        delete m_loaders.take(appId);
    }
}

// ═══════════════════════════════════════════════════════════════
// 启停
// ═══════════════════════════════════════════════════════════════

bool StandaloneManager::startNode(const QString &appId)
{
    for (auto &info : m_nodes) {
        if (info.appId != appId) continue;

        if (info.exePath.isEmpty()) {
            emit nodeError(appId, "No executable path");
            return false;
        }

        QString exe = info.absoluteExePath();
        if (!QFile::exists(exe)) {
            emit nodeError(appId, QString("Executable not found: %1").arg(exe));
            return false;
        }

        NodeLoader *loader = getOrCreateLoader(appId);
        if (loader->statuscheck()) {
            qDebug() << "[Standalone] Already running:" << appId;
            return false;
        }

        loader->start(exe, QStringList());
        if (loader->statuscheck()) {
            info.isOnline = true;
            emit nodeStarted(appId);
            return true;
        }
        return false;
    }
    return false;
}

bool StandaloneManager::stopNode(const QString &appId)
{
    if (!m_loaders.contains(appId)) {
        // 尝试按进程名杀
        for (const auto &info : m_nodes) {
            if (info.appId != appId) continue;
            if (info.exePath.isEmpty()) return false;

            QString exeName = QFileInfo(info.absoluteExePath()).fileName();
            QProcess::execute("taskkill", QStringList()
                              << "/F" << "/T" << "/IM" << exeName);
            for (auto &n : m_nodes) {
                if (n.appId == appId) n.isOnline = false;
            }
            emit nodeStopped(appId);
            return true;
        }
        return false;
    }

    NodeLoader *loader = m_loaders[appId];
    if (!loader->statuscheck()) return true;

    loader->stop();
    if (!loader->statuscheck()) {
        for (auto &n : m_nodes) {
            if (n.appId == appId) n.isOnline = false;
        }
        emit nodeStopped(appId);
        return true;
    }
    return false;
}

bool StandaloneManager::isNodeRunning(const QString &appId) const
{
    return m_loaders.contains(appId) && m_loaders[appId]->statuscheck();
}

void StandaloneManager::startAutoStartNodes()
{
    for (const auto &n : m_nodes) {
        if (n.autoStart && !n.exePath.isEmpty())
            startNode(n.appId);
    }
}

// ═══════════════════════════════════════════════════════════════
// KnotLink 消息处理 — 独立 socketID: 0x00000012
// ═══════════════════════════════════════════════════════════════

void StandaloneManager::onKnotLinkData(const QString &data, const QString &questionID)
{
    KLKVMap kvMap;
    kvMap.deserialize(data);

    qDebug() << "[Standalone] KL data:" << kvMap;

    QString cmd = kvMap["cmd"];
    QString reply;

    if (cmd == "get_standalone_list") {
        reply = QString::fromUtf8(exportListToJson());

    } else if (cmd == "get_detail") {
        QString appId = kvMap["plugin_name"];
        NodeInfo info = nodeInfo(appId);
        if (!info.isValid()) {
            reply = "Error: standalone node not found";
        } else {
            QJsonObject obj;
            obj["plugin_name"] = info.appId;
            obj["app_id"]      = info.appId;
            obj["name"]        = info.appName.isEmpty() ? info.appId : info.appName;
            obj["author"]      = info.author;
            obj["version"]     = info.version;
            obj["description"] = info.description;
            obj["status"]      = info.isOnline ? "运行中" : "停止";
            obj["auto_start"]  = info.autoStart ? "true" : "false";
            obj["exe_path"]    = info.exePath;
            obj["node_type"]   = "standalone";
            reply = QString::fromUtf8(QJsonDocument(obj).toJson());
        }

    } else if (cmd == "standalone_control") {
        QString action      = kvMap["action"];
        QString appId       = kvMap["plugin_name"];
        bool    success     = false;
        QString errorMsg;

        if (action == "start") {
            success = startNode(appId);
            if (!success) errorMsg = "Failed to start: " + appId;
        } else if (action == "stop") {
            success = stopNode(appId);
            if (!success) errorMsg = "Failed to stop: " + appId;
        } else if (action == "restart") {
            stopNode(appId);
            QThread::msleep(100);
            success = startNode(appId);
            if (!success) errorMsg = "Failed to restart: " + appId;
        } else {
            errorMsg = "Unsupported action: " + action;
        }

        reply = success ? "ok" : ("error: " + errorMsg);

    } else if (cmd == "get_funclist") {
        QString appId = kvMap["plugin_name"];
        QByteArray content = getFuncList(appId);
        reply = content.isEmpty()
                ? "Error: FuncList.json not found for " + appId
                : QString::fromUtf8(content);

    } else if (cmd == "update_config") {
        // 预留：未来可修改独立节点 autostart
        reply = "ok";

    } else if (cmd == "refresh") {
        scan();
        reply = QString::fromUtf8(exportListToJson());

    } else {
        reply = "Error: unknown command: " + cmd;
    }

    qDebug() << reply;

    m_responder->sendBack(reply, questionID);
}
