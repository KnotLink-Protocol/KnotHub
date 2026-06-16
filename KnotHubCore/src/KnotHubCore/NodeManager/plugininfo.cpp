#include "plugininfo.h"
#include <QDir>
#include <QJsonValue>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

QString PluginInfo::absoluteExePath() const
{
    if (exePath.isEmpty()) return QString();
    // exePath 可能以 / 或 \ 开头，QDir 会自动处理
    return QDir(folderPath).filePath(exePath);
}

PluginInfo PluginInfo::fromJson(const QJsonObject &obj, const QString &folderPath)
{
    PluginInfo info;
    info.folderPath = folderPath;
    info.pluginName = obj.value("plugin_name").toString();
    info.appId = obj.value("app_id").toString();
    info.author = obj.value("author").toString();
    info.description = obj.value("description").toString();
    info.autoStart = obj.value("auto_start").toString().compare("true", Qt::CaseInsensitive) == 0;
    info.exePath = obj.value("exe_path").toString();
    info.version = obj.value("version").toString();
    return info;
}

QByteArray PluginInfo::toJson(QString status)
{
    QJsonObject obj;
    obj["plugin_name"] = pluginName;
    obj["app_id"] = appId;
    obj["author"] = author;
    obj["description"] = description;
    obj["auto_start"] = autoStart ? "true" : "false";
    obj["exe_path"] = exePath;
    obj["version"] = version;
    obj["status"] = status;
    QJsonDocument doc(obj);
    QByteArray jsonData = doc.toJson();
    return jsonData;
}

bool PluginInfo::isValid() const
{
    return !pluginName.isEmpty() && !exePath.isEmpty() && QFile::exists(absoluteExePath());
}
