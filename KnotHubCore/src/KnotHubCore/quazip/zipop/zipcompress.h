#ifndef ZIPCOMPRESS_H
#define ZIPCOMPRESS_H

#include "JlCompress.h"
#include "quazip_global.h"
#include <QObject>

namespace compressionTool {
class zipCompressPrivate;

class QUAZIP_EXPORT zipCompress : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(zipCompress)
    Q_DECLARE_PRIVATE(zipCompress)

public:
    explicit zipCompress(QObject *parent = nullptr);
    ~zipCompress();

    /**
     * @brief 获取压缩包的文件列表
     * @param fileCompressed 压缩包名
     * @return 压缩包内的文件列表
     */
    QStringList getFileList(QString fileCompressed);

    /**
     * @brief 同步压缩文件
     * @param fileCompressed 压缩包名
     * @param files 需要压缩的文件名列表
     * @return true 表示压缩成功，false 表示压缩失败
     */
    bool syncCompressFiles(QString fileCompressed, QStringList files);

    /**
     * @brief 同步解压文件目录
     * @param fileCompressed 压缩包名
     * @param dir 解压后的文件输出路径
     * @return 解压出的文件列表
     */
    QStringList syncExtractDir(QString fileCompressed, QString dir);

    /**
     * @brief 同步压缩文件目录
     * @param fileCompressed 压缩包名
     * @param dir 要压缩的目录路径
     * @param recursive 是否递归压缩子目录
     * @return true 表示压缩成功，false 表示压缩失败
     */
    bool syncCompressDir(QString fileCompressed, QString dir, bool recursive = true);

    /**
     * @brief 同步解压文件
     * @param fileCompressed 压缩包名
     * @param files 要解压的文件列表
     * @param dir 解压后的文件输出路径
     * @return 解压出的文件列表
     */
    QStringList syncExtractFiles(QString fileCompressed, QStringList files);


    /**
     * @brief 加密压缩文件夹内的所有文件
     * @param fileCompressed    压缩包名
     * @param dir               压缩路径
     * @param password          密码
     * @return
     */
    bool compressDirsWithPassword(const QString &fileCompressed,const QString &dir,const QString& password);

    /**
     * @brief 加密压缩多个文件
     * @param fileCompressed        压缩包名
     * @param files                 要解压的文件列表
     * @param password              密码
     * @return
     */
    bool compressFilesWithPassword(const QString &fileCompressed,const QStringList &files,const QString& password);

    /**
     * @brief 加密解压缩zip
     * @param fileCompressed        压缩包名
     * @param dir                   解压后的路径
     * @param password
     * @return
     */
    bool extractDirWithPassword(const QString &fileCompressed, const QString &dir, const QString& password);

private:
    /**
     * @brief 获得 directoryPath 路径下的所有文件和子文件，不包括空文件夹
     * @param directoryPath
     * @return 文件路径
     */
    QStringList listFilesInDirectoryRecursively(const QString &directoryPath);


Q_SIGNALS:
    /**
     * @brief (加解密)解压缩进度通知信号
     * @param progress 进度值，范围为 0 到 100
     * @param operateType 操作类型（1压缩或2解压）
     * @param filePath 当前正操作的文件
     */
    void signalCompressionProgress(qreal progress, QString operateType, QString filePath);

    /**
     * @brief (加解密)解压缩处理的当前文件进度通知信号
     * @param progress  进度值，范围为 0 到 100
     * @param filePath  当前正操作的文件
     */
    void signalFileProgress(qreal progress, QString filePath);

    /**
     * @brief (加密)压缩文件目录结束信号
     * @param status 压缩操作是否成功
     */
    void signalCompressDirinish(bool status);

    /**
     * @brief (解密)解压文件目录结束信号
     * @param status 解压操作是否成功
     */
    void signalExtractDirFinish(bool status);

private:
    JlCompress mJlCompress;
    const uint32_t mBlockSize = 4096;  // 文件分块读写大小 (一页)

};

} // namespace compressionTool

#endif // ZIPCOMPRESS_H

