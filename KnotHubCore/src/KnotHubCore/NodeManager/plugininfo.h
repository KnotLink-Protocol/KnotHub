#ifndef PLUGININFO_H
#define PLUGININFO_H

#include <QString>
#include <QJsonObject>

struct PluginInfo
{
    QString pluginName;      // plugin_name
    QString appId;           // app_id (十六进制字符串，如 "0x00000001")
    QString author;
    QString description;
    bool autoStart;          // auto_start
    QString exePath;         // 相对于插件文件夹的路径，如 "/testplugin.exe"
    QString folderPath;      // 插件文件夹的绝对路径
    QString version;

    // 获取 exe 的绝对路径
    QString absoluteExePath() const;

    // 从 JSON 对象解析
    static PluginInfo fromJson(const QJsonObject &obj, const QString &folderPath);
    QByteArray toJson();

    // 验证有效性
    bool isValid() const;
};

#endif // PLUGININFO_H
