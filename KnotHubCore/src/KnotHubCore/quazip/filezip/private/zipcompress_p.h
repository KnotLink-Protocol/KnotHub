#ifndef ZIPCOMPRESS_P_H
#define ZIPCOMPRESS_P_H

#include "zipcompressthread.h"
#include "zipcompress.h"

#include <qobject_p.h>
#include <private/qobject_p.h>
#include <QScopedPointer>

namespace compressionTool {
class zipCompressPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(zipCompress)
public:
    zipCompressPrivate(zipCompress *ptr);
    ~zipCompressPrivate();
    //! 初始化化信号槽
    void initConnect();
    //! 获取压缩包的文件列表
    QStringList getFileList(QString fileCompressed);
    //! 异步/同步压缩文件（包含单，多文件）
    bool fileCompressFiles(QString fileCompressed, QStringList files, bool isSync = false);
    //! 异步/同步解压缩文件夹
    QStringList fileExtractDir(QString fileCompressed, QString dir = QString(), bool isSync = false);
    //! 异步/同步压缩文件夹
    bool fileCompressDir(QString fileCompressed, QString dir = QString(), bool isSync = false, bool recursive = true);
    //! 异步/同步解压缩文件
    QStringList fileExtractFiles(QString fileCompressed, QStringList files={}, QString dir = QString(), bool isSync = false);
private:
    //! 数据加解压对象
    QScopedPointer<zipCompressThread> m_zipCompressThread;
};
}
#endif // ZIPCOMPRESS_P_H
