#ifndef KLUDF_H
#define KLUDF_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QDebug>

class KLUDF
{
public:
    KLUDF();
};

class KLKVMap : public QMap<QString, QString>
{
public:
    // 将 KLKVMap 序列化为键值对字符串
    QString serialize() const;

    // 将键值对字符串反序列化为 KVMap
    void deserialize(const QString& keyValueString);

    // 安全地读取键值对
    QString get(const QString& key) const;
};


#endif // KLUDF_H
