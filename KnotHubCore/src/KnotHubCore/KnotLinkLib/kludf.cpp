#include "kludf.h"

KLUDF::KLUDF()
{

}

// 将 KVMap 序列化为键值对字符串
QString KLKVMap::serialize() const
{
    QStringList pairs;
    for (const auto& key : this->keys())
    {
        pairs.append(key + "=" + this->value(key));
    }
    return pairs.join(";");
}

// 将键值对字符串反序列化为 KVMap
void KLKVMap::deserialize(const QString& keyValueString)
{
    this->clear(); // 清空当前映射
    QStringList pairs = keyValueString.split(";");
    for (const auto& pair : pairs)
    {
        QStringList keyValue = pair.split("=", QString::SkipEmptyParts);
        if (keyValue.size() == 2)
        {
            this->insert(keyValue[0], keyValue[1]);
        }
    }
}

// 安全地读取键值对
QString KLKVMap::get(const QString& key) const
{
    if (this->contains(key))
    {
        return this->value(key);
    }
    else
    {
        return QString(); // 返回空字符串
    }
}



