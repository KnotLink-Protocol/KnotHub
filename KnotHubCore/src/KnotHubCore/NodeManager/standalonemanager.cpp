#include "standalonemanager.h"
#include <KnotLinkLib>

#include <QDebug>

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
}

// ═══════════════════════════════════════════════════════════════
// 扫描 — 读取注册表 HKCU\Software\KnotLink\StandaloneNodes
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
        emit nodeListChanged();
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

        m_nodes.append(info);

        qDebug() << "[Standalone] Node added:" << info.appId << info.appName;
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
        obj["status"]      = "已注册";            // 独立式不管理运行状态
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
// KnotLink 消息处理 — socketID: 0x00000012
// ═══════════════════════════════════════════════════════════════

void StandaloneManager::onKnotLinkData(const QString &data, const QString &questionID)
{
    KLKVMap kvMap;
    kvMap.deserialize(data);

    qDebug() << "[Standalone] KL data:" << kvMap;

    QString cmd = kvMap["cmd"];
    QString reply;

    if (cmd == "get_standalone_list") {
        scan();  // 动态扫描注册表
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
            obj["status"]      = "已注册";
            obj["auto_start"]  = info.autoStart ? "true" : "false";
            obj["exe_path"]    = info.exePath;
            obj["node_type"]   = "standalone";
            reply = QString::fromUtf8(QJsonDocument(obj).toJson());
        }

    } else if (cmd == "get_funclist") {
        QString appId = kvMap["plugin_name"];
        QByteArray content = getFuncList(appId);
        reply = content.isEmpty()
                ? "Error: FuncList.json not found for " + appId
                : QString::fromUtf8(content);

    } else if (cmd == "refresh") {
        scan();
        reply = QString::fromUtf8(exportListToJson());

    } else {
        reply = "Error: unknown command: " + cmd;
    }

    qDebug() << reply;

    m_responder->sendBack(reply, questionID);
}
