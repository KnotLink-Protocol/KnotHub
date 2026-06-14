#ifndef ZIPCOMPRESSTHREAD_H
#define ZIPCOMPRESSTHREAD_H
#include "quazip.h"

#include <atomic>

#include <QThread>
#include <QDir>

namespace compressionTool {
class zipCompressThread : public QThread
{
    Q_OBJECT
public:
    //! 加解压的数据类型
    enum ZipCompressType{
        Unknown = 0,
        CompressFiles,
        ExtractFiles,
        CompressDir,
        ExtractDir,

    };
    Q_ENUM(ZipCompressType);
    explicit zipCompressThread(QObject *parent = nullptr);
    //! 异步/同步压缩文件（包含单，多文件）
    bool fileCompressFiles(QString fileCompressed, QStringList files, bool isSync = false);
    //! 异步/同步解压缩文件夹
    QStringList fileExtractDir(QString fileCompressed, QString dir = QString(), bool isSync = false);
    //! 异步/同步压缩文件夹
    bool fileCompressDir(QString fileCompressed, QString dir = QString(), bool isSync = false, bool recursive = true);
    //! 异步/同步解压缩文件
    QStringList fileExtractFiles(QString fileCompressed, QStringList files={}, QString dir = QString(), bool isSync = false);
    //! 获取压缩包的文件列表
    QStringList getFileList(QString fileCompressed);
protected:
    //! 在线程中运行解压缩功能
    virtual void run();
private:
    //! 枚举转字符串
    QString enumToString(ZipCompressType value);
    //! 压缩文件目录
    bool fileCompressDir(const bool &isSync = false );
    //! 压缩文件
    bool fileCompressFiles(const bool &isSync= false);
    //! 解压文件目录
    QStringList fileExtractDir(const bool &isSync = false);
    //! 解压文件
    QStringList fileExtractFiles(const bool &isSync = false);
    //! 文件数据操作
    bool copyData(QIODevice &inFile, QIODevice &outFile,const QString  &fileName = QString());
    //! 计算未压缩的文件大小
    qint64 calculateFilesSize(const QStringList& files);
    //! 计算未压缩的文件目录大小
    qint64 calculateFolderSize(const QString& folderPath);
    //! 检查操作
    bool checkOperation(ZipCompressType type, QString fileCompressed);
    //! 设置压缩操作
    bool setupCompressOperation(ZipCompressType type, bool isSync);
    //! 设置解压操作
    QStringList setupExtractOperation(ZipCompressType type, bool isSync);
    //! 计算解压后文件大小
    quint64 getUncompressedSize(const QString &zipFilePath,
                                const QStringList &fileNames = QStringList());

private:
    QStringList extractDir(QuaZip &zip, const QString &dir);
    QStringList getFileList(QuaZip *zip);
    QString extractFile(QuaZip &zip, QString fileName, QString fileDest);
    QStringList extractFiles(QuaZip &zip, const QStringList &files, const QString &dir);
    /// Compress a single file.
    /**
      \param zip Opened zip to compress the file to.
      \param fileName The full path to the source file.
      \param fileDest The full name of the file inside the archive.
      \return true if success, false otherwise.
      */
    bool compressFile(QuaZip* zip, QString fileName, QString fileDest);
    /// Compress a subdirectory.
    /**
      \param parentZip Opened zip containing the parent directory.
      \param dir The full path to the directory to pack.
      \param parentDir The full path to the directory corresponding to
      the root of the ZIP.
      \param recursive Whether to pack sub-directories as well or only
      files.
      \return true if success, false otherwise.
      */
    bool compressSubDir(QuaZip* parentZip, QString dir, QString parentDir, bool recursive,
                        QDir::Filters filters);
    /// Extract a single file.
    /**
      \param zip The opened zip archive to extract from.
      \param fileName The full name of the file to extract.
      \param fileDest The full path to the destination file.
      \return true if success, false otherwise.
      */
    bool extractFile(QuaZip* zip, QString fileName, QString fileDest);
    /// Remove some files.
    /**
      \param listFile The list of files to remove.
      \return true if success, false otherwise.
      */
    bool removeFile(QStringList listFile);

    /// Compress a single file.
    /**
      \param fileCompressed The name of the archive.
      \param file The file to compress.
      \return true if success, false otherwise.
      */
    bool compressFile(QString fileCompressed, QString file);
    /// Compress a list of files.
    /**
      \param fileCompressed The name of the archive.
      \param files The file list to compress.
      \return true if success, false otherwise.
      */
    bool compressFiles(QString fileCompressed, QStringList files);
    /// Compress a whole directory.
    /**
      Does not compress hidden files. See compressDir(QString, QString, bool, QDir::Filters).

      \param fileCompressed The name of the archive.
      \param dir The directory to compress.
      \param recursive Whether to pack the subdirectories as well, or
      just regular files.
      \return true if success, false otherwise.
      */
    bool compressDir(QString fileCompressed, QString dir = QString(), bool recursive = true);
    /**
     * @brief Compress a whole directory.
     *
     * Unless filters are specified explicitly, packs
     * only regular non-hidden files (and subdirs, if @c recursive is true).
     * If filters are specified, they are OR-combined with
     * <tt>%QDir::AllDirs|%QDir::NoDotAndDotDot</tt> when searching for dirs
     * and with <tt>QDir::Files</tt> when searching for files.
     *
     * @param fileCompressed path to the resulting archive
     * @param dir path to the directory being compressed
     * @param recursive if true, then the subdirectories are packed as well
     * @param filters what to pack, filters are applied both when searching
     * for subdirs (if packing recursively) and when looking for files to pack
     * @return true on success, false otherwise
     */
    bool compressDir(QString fileCompressed, QString dir,
                     bool recursive, QDir::Filters filters);

    /// Extract a single file.
    /**
      \param fileCompressed The name of the archive.
      \param fileName The file to extract.
      \param fileDest The destination file, assumed to be identical to
      \a file if left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QString extractFile(QString fileCompressed, QString fileName, QString fileDest = QString());
    /// Extract a list of files.
    /**
      \param fileCompressed The name of the archive.
      \param files The file list to extract.
      \param dir The directory to put the files to, the current
      directory if left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractFiles(QString fileCompressed, QStringList files, QString dir = QString());
    /// Extract a whole archive.
    /**
      \param fileCompressed The name of the archive.
      \param dir The directory to extract to, the current directory if
      left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractDir(QString fileCompressed, QString dir = QString());
    /// Extract a whole archive.
    /**
      \param fileCompressed The name of the archive.
      \param fileNameCodec The codec to use for file names.
      \param dir The directory to extract to, the current directory if
      left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractDir(QString fileCompressed, QTextCodec* fileNameCodec, QString dir = QString());
    /// Extract a single file.
    /**
      \param ioDevice pointer to device with compressed data.
      \param fileName The file to extract.
      \param fileDest The destination file, assumed to be identical to
      \a file if left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QString extractFile(QIODevice *ioDevice, QString fileName, QString fileDest = QString());
    /// Extract a list of files.
    /**
      \param ioDevice pointer to device with compressed data.
      \param files The file list to extract.
      \param dir The directory to put the files to, the current
      directory if left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractFiles(QIODevice *ioDevice, QStringList files, QString dir = QString());
    /// Extract a whole archive.
    /**
      \param ioDevice pointer to device with compressed data.
      \param dir The directory to extract to, the current directory if
      left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractDir(QIODevice *ioDevice, QString dir = QString());
    /// Extract a whole archive.
    /**
      \param ioDevice pointer to device with compressed data.
      \param fileNameCodec The codec to use for file names.
      \param dir The directory to extract to, the current directory if
      left empty.
      \return The list of the full paths of the files extracted, empty on failure.
      */
    QStringList extractDir(QIODevice* ioDevice, QTextCodec* fileNameCodec, QString dir = QString());
    /// Get the file list.
    /**
      \return The list of the files in the archive, or, more precisely, the
      list of the entries, including both files and directories if they
      are present separately.
      */
    QStringList getFileList(QIODevice *ioDevice);
Q_SIGNALS:
    //! 解压缩进度通知信号
    void signalCompressionProgress(qreal progress,QString  operateType,QStringList fileNames);
    //! 压缩文件结束信号
    void signalCompressFilesFinish(bool status);
    //! 压缩文件夹结束信号
    void signalCompressDirinish(bool status);
    //! 解压文件结束信号
    void  signalExtractFilesFinish(QStringList info);
    //! 解压文件夹结束信号
    void  signalExtractDirFinish(QStringList info);
private:
    //! 当前读取的总数量
    qint64 m_totalBytesRead = 0;
    //! 数据总大小
    qint64 m_totalSize = 0;
    //! 加解压的数据类型
    ZipCompressType m_compressType = Unknown;
    //! 未压缩文件地址
    QStringList m_compressedFilesUrl;
    //! 未压缩目录地址
    QString m_compressedDirUrl;
    //! 压缩文件地址
    QString m_fileCompressed;
    //! 是否递归子目录
    bool m_dirRecursive = true;
    //! 解压文件列表
    QStringList m_extractFiles;
    //! 目标文件夹
    QString m_extractDir;
    //! 是否执行操作
    std::atomic<bool> m_run;
    //! 加解压文件信息
    QStringList m_fileInfo;
};
}
#endif // ZIPCOMPRESSTHREAD_H
