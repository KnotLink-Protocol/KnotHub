#ifndef ZIPCOMPRESS_H
#define ZIPCOMPRESS_H

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
     * @brief 异步压缩文件
     * @param fileCompressed 压缩包名
     * @param files 需要压缩的文件名列表
     * @details 需要关联 signalCompressFilesFinish 信号获取压缩操作结果
     */
    void asyncCompressFiles(QString fileCompressed, QStringList files);

    /**
     * @brief 异步解压文件目录
     * @param fileCompressed 压缩包名
     * @param dir 解压后的文件输出路径
     * @details 需要关联 signalExtractDirFinish 信号获取解压操作结果
     */
    void asyncExtractDir(QString fileCompressed, QString dir);

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
     * @brief 异步压缩文件目录
     * @param fileCompressed 压缩包名
     * @param dir 要压缩的目录路径
     * @param recursive 是否递归压缩子目录
     * @details 需要关联 signalCompressDirinish 信号获取压缩操作结果
     */
    void asyncCompressDir(QString fileCompressed, QString dir, bool recursive = true);

    /**
     * @brief 异步解压文件
     * @param fileCompressed 压缩包名
     * @param files 要解压的文件列表
     * @param dir 解压后的文件输出路径
     * @details 需要关联 signalExtractFilesFinish 信号获取解压操作结果
     */
    void asyncExtractFiles(QString fileCompressed, QStringList files, QString dir);

    /**
     * @brief 同步解压文件
     * @param fileCompressed 压缩包名
     * @param files 要解压的文件列表
     * @param dir 解压后的文件输出路径
     * @return 解压出的文件列表
     */
    QStringList syncExtractFiles(QString fileCompressed, QStringList files, QString dir);

Q_SIGNALS:
    /**
     * @brief 解压缩进度通知信号
     * @param progress 进度值，范围为 0.0 到 1.0
     * @param operateType 操作类型（压缩或解压）
     * @param fileNames 当前操作的文件列表
     */
    void signalCompressionProgress(qreal progress, QString operateType, QStringList fileNames);

    /**
     * @brief 压缩文件结束信号
     * @param status 压缩操作是否成功
     */
    void signalCompressFilesFinish(bool status);

    /**
     * @brief 压缩文件目录结束信号
     * @param status 压缩操作是否成功
     */
    void signalCompressDirinish(bool status);

    /**
     * @brief 解压文件结束信号
     * @param info 解压出的文件列表
     */
    void signalExtractFilesFinish(QStringList info);

    /**
     * @brief 解压文件目录结束信号
     * @param info 解压出的文件列表
     */
    void signalExtractDirFinish(QStringList info);
};

} // namespace compressionTool

#endif // ZIPCOMPRESS_H

