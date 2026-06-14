#include "zipcompress.h"

#include <QDirIterator>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QThread>

namespace compressionTool {
zipCompress::zipCompress(QObject *parent):
    QObject(parent)
{

}
zipCompress::~zipCompress()
{

}

QStringList zipCompress::getFileList(QString fileCompressed)
{
    return  mJlCompress.getFileList(fileCompressed);
}
bool zipCompress::syncCompressFiles(QString fileCompressed, QStringList files)
{
    return  mJlCompress.compressFiles(fileCompressed,files);
}

QStringList zipCompress::syncExtractDir(QString fileCompressed, QString dir)
{
    return mJlCompress.extractDir(fileCompressed, dir);
}

bool zipCompress::syncCompressDir(QString fileCompressed, QString dir, bool recursive)
{
    return mJlCompress.compressDir(fileCompressed, dir, recursive);
}

QStringList zipCompress::syncExtractFiles(QString fileCompressed, QStringList files)
{
    return mJlCompress.extractFiles(fileCompressed, files);
}


bool zipCompress::compressDirsWithPassword(const QString &fileCompressed, const QString &dir, const QString &password) {
    bool result = false;

    if (fileCompressed.isEmpty() || dir.isEmpty()) {
        emit signalCompressDirinish(result);
        return result;
    }

    QuaZip newZip(fileCompressed); // 要生成的zip文件
    if (!newZip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create ZIP file:" << fileCompressed;
        emit signalCompressDirinish(result);
        return result;
    }

    // 读取目录中的所有文件
    QStringList filePathList = listFilesInDirectoryRecursively(dir);
    if (filePathList.isEmpty()) {
        qDebug() << "No files to compress in directory:" << dir;
        newZip.close();
        emit signalCompressDirinish(result);
        return result;
    }

    qreal pro = 100.0 / filePathList.size();
    qreal progress = 1.0;

    QuaZipFile zipFile(&newZip);
    for (const QString &filePath : filePathList) {
        // 构造相对于dir的文件名
        QString fileName = QDir(dir).relativeFilePath(filePath);

        // 创建ZIP文件条目信息
        QuaZipNewInfo info(fileName, filePath);

        // 尝试打开ZIP文件条目进行写入
        if (!zipFile.open(QIODevice::WriteOnly, info, password.toUtf8().constData(), 0, 8)) {
            qDebug() << "Failed to open ZIP entry for writing:" << fileName;
            // 关闭ZIP
            newZip.close();
            emit signalCompressDirinish(result);
            return result;
        }

        // 打开源文件进行读取
        QFile sourceFile(filePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open source file for reading:" << filePath;
            // 关闭ZIP
            newZip.close();
            emit signalCompressDirinish(result);
            return result;
        }

        // 总进度
        emit signalCompressionProgress(progress, "1", filePath);

        // 大文件读取分块处理，否则程序会闪退
        {
            qreal file_pro = 100.0 / (sourceFile.size() / mBlockSize);
            qreal file_progress = 0.0;

            // 将源文件的内容写入ZIP文件
            QByteArray buffer;
            while(!sourceFile.atEnd()) {
                buffer = sourceFile.read(mBlockSize);
                zipFile.write(buffer);

                file_progress += file_pro;
                emit signalFileProgress(file_progress, filePath);
            }

            emit signalFileProgress(100, filePath);
        }

        sourceFile.close();
        zipFile.close();

        progress += pro;
        progress = qBound(0.0, progress, 99.999);
    }


    result = true;
    newZip.close();

    emit signalCompressionProgress(100, "1", "");
    emit signalCompressDirinish(result);

    return result;
}


bool zipCompress::compressFilesWithPassword(const QString &fileCompressed, const QStringList &files, const QString &password) {
    bool result = false;
    if (fileCompressed.isEmpty() || files.isEmpty()) {
        emit signalCompressDirinish(result);
        return result;
    }

    QuaZip newZip(fileCompressed); // 要生成的zip文件
    if (!newZip.open(QuaZip::mdCreate)) {
        qDebug() << "Failed to create ZIP file:" << fileCompressed;
        emit signalCompressDirinish(result);
        return result;
    }

    qreal pro = 100.0 / files.size();
    qreal progress = 1.0;
    emit signalCompressionProgress(progress, "1", "");

    QuaZipFile zipFile(&newZip);
    for (const QString &filePath : files) {
        QFileInfo fileInfo(filePath);

        // 创建ZIP文件条目信息
        QuaZipNewInfo info(fileInfo.fileName(), filePath);

        // 尝试打开ZIP文件条目进行写入
        if (!zipFile.open(QIODevice::WriteOnly, info, password.toUtf8().constData())) {
            qDebug() << "Failed to open ZIP entry for writing:" << fileInfo.fileName();
            newZip.close();
            QFile(fileCompressed).remove();
            emit signalCompressDirinish(result);
            return result;
        }

        // 打开源文件进行读取
        QFile sourceFile(filePath);
        if (!sourceFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open source file for reading:" << filePath;
            zipFile.close();
            emit signalCompressDirinish(result);
            return result;

        }

        // 进度
        emit signalCompressionProgress(progress, "1", filePath);

        // 大文件读取分块处理，否则程序会闪退
        {
            qreal file_pro = 100.0 / (sourceFile.size() / mBlockSize);
            qreal file_progress = 0.0;

            // 将源文件的内容写入ZIP文件
            QByteArray buffer;
            while(!sourceFile.atEnd()) {
                buffer = sourceFile.read(mBlockSize);
                zipFile.write(buffer);

                file_progress += file_pro;
                emit signalFileProgress(file_progress, filePath);
            }

            emit signalFileProgress(100, filePath);
        }

        sourceFile.close();
        zipFile.close();

        progress += pro;
        progress = qBound(0.0, progress, 99.999);
    }

    result = true;
    newZip.close();

    emit signalCompressionProgress(100, "1", "");
    emit signalCompressDirinish(result);

    return result;
}


bool zipCompress::extractDirWithPassword(const QString &fileCompressed, const QString &dir, const QString &password) {
    bool result = false;

    if (fileCompressed.isEmpty() || dir.isEmpty()) {
        emit signalExtractDirFinish(result);
        return result;
    }

    // 使用QFileInfo获取文件名（不带扩展名）并构建目标目录路径
    QFileInfo fileInfo(fileCompressed);
    QString targetDirName = fileInfo.completeBaseName();
    QString targetDirPath = QDir(dir).absoluteFilePath(targetDirName);

    // 创建目标目录（如果不存在）
    QDir targetDir(targetDirPath);
    if (!targetDir.exists() && !targetDir.mkpath(".")) {
        qDebug() << "Failed to create target directory:" << targetDirPath;
        emit signalExtractDirFinish(result);
        return result;
    }

    // 打开ZIP文件
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Failed to open ZIP file:" << fileCompressed;
        emit signalExtractDirFinish(result);
        return result;
    }

    // 遍历ZIP文件中的所有条目
    QList<QuaZipFileInfo> fileInfoList = zip.getFileInfoList();

    qreal pro = 100.0 / fileInfoList.size();
    qreal progress = 1.0;

    for (const QuaZipFileInfo &fileInfoEntry : fileInfoList) {
        // 组合目标文件路径
        QString fileName = fileInfoEntry.name;
        QString filePath = targetDir.absoluteFilePath(fileName);

        // 如果是文件夹，如果不存在则创建；然后跳过文件夹的操作
        QFileInfo info(filePath);
        if (info.isDir()) {
            QDir().mkpath(filePath);
            continue;

        } else {    // 如果是文件，如果路径不存在，创建该路径
            QString path = info.path();
            QDir dpath(path);
            if (!dpath.exists()) {
                if (!dpath.mkpath(path)) {
                    qDebug() << "Failed to create directory:" << path;
                    emit signalExtractDirFinish(result);
                    return result;
                }
            }
        }


        // 打开ZIP条目并解压文件
        QuaZipFile zipFile(zip.getZipName(), fileName);
        if (zipFile.open(QIODevice::ReadOnly, password.toUtf8().constData())) {
            QFile outFile(filePath);
            if (!outFile.open(QIODevice::WriteOnly)) {
                qDebug() << "Failed to open output file for writing:" << filePath;
                zipFile.close();
                zip.close();
                emit signalExtractDirFinish(result);
                return result;
            }

            emit signalCompressionProgress(progress, "2", filePath);

            // 大文件读取分块处理，否则程序会闪退
            {
                qreal file_pro = 100.0 / (zipFile.size() / mBlockSize);
                qreal file_progress = 0.0;

                // 解压并写入文件
                QByteArray buffer;
                while(!zipFile.atEnd()) {
                    buffer = zipFile.read(mBlockSize);
                    outFile.write(buffer);

                    file_progress += file_pro;
                    emit signalFileProgress(file_progress, filePath);
                }

                emit signalFileProgress(100, filePath);
            }

            outFile.close();
            zipFile.close();
        } else {
            qDebug() << "Failed to open ZIP entry for reading:" << fileName;
            zip.close();
            emit signalExtractDirFinish(result);
            return result;
        }

        progress += pro;
        progress = qBound(0.0, progress, 99.999);
    }


    result = true;
    // 关闭ZIP文件
    zip.close();

    emit signalCompressionProgress(100, "2", "");
    emit signalExtractDirFinish(result);

    return result;
}




QStringList zipCompress::listFilesInDirectoryRecursively(const QString &directoryPath) {
    QStringList list;

    QDirIterator it(directoryPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        list.append(filePath);
    }

    return list;
}



}
