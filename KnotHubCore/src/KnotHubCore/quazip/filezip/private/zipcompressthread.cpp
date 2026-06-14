#include "zipcompressthread.h"
#include "quazipfile.h"
#include <QDebug>
#include <QMetaEnum>
#include <QElapsedTimer>
namespace compressionTool {
zipCompressThread::zipCompressThread(QObject *parent):QThread(parent)
  , m_run(false)
  ,m_totalBytesRead(0)
  ,m_totalSize(0)
{

}
bool zipCompressThread::fileCompressFiles(QString fileCompressed, QStringList files, bool isSync)
{
    if(!checkOperation(CompressFiles,fileCompressed)){
        return false;
    }
    m_compressedFilesUrl = files;
    return  setupCompressOperation(CompressFiles, isSync);
}

bool zipCompressThread::fileCompressDir(QString fileCompressed, QString dir , bool isSync, bool recursive)
{
    if(!checkOperation(CompressDir,fileCompressed)){
        return false;
    }
    m_compressedDirUrl = dir;
    m_dirRecursive = recursive;
    return setupCompressOperation(CompressDir,isSync);
}

QStringList zipCompressThread::fileExtractFiles(QString fileCompressed, QStringList files, QString dir, bool isSync)
{
    if(!checkOperation(ExtractFiles,fileCompressed)){
        return QStringList();
    }
    m_extractFiles = files;
    m_extractDir = dir;
    return setupExtractOperation(ExtractFiles, isSync);
}

QStringList zipCompressThread::fileExtractDir(QString fileCompressed, QString dir, bool isSync)
{
    if(!checkOperation(ExtractDir,fileCompressed)){
        return QStringList();
    }
    m_extractDir = dir;
    return setupExtractOperation(ExtractDir, isSync);
}
void zipCompressThread::run()
{
    switch(m_compressType)
    {
    case CompressFiles:
        fileCompressFiles();
        break;
    case ExtractFiles:
        fileExtractFiles();
        break;
    case CompressDir:
        fileCompressDir();
        break;
    case ExtractDir:
        fileExtractDir();
        break;
    default:
        qWarning() << " not found operation type";
        break;
    }

}
QString zipCompressThread::enumToString(ZipCompressType value)
{
    return staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("ZipCompressType")).valueToKey(value);
}

bool zipCompressThread::fileCompressDir(const bool &isSync)
{
    //! 压缩目录到指定目录
    m_fileInfo.append(m_compressedDirUrl);
    m_totalSize = calculateFolderSize(m_compressedDirUrl);
    auto status =  compressDir(m_fileCompressed, m_compressedDirUrl,m_dirRecursive);
    m_run = false;
    if(isSync)
        return status;
    Q_EMIT signalCompressDirinish(status);
}

bool zipCompressThread::fileCompressFiles(const bool &isSync)
{
    //! 压缩指定文件到指定目录
    m_fileInfo = m_compressedFilesUrl;
    m_totalSize = calculateFilesSize(m_compressedFilesUrl);
    auto status = compressFiles(m_fileCompressed, m_compressedFilesUrl);
    m_run = false;
    if(isSync)
        return status;
    Q_EMIT signalCompressFilesFinish(status);
}

QStringList zipCompressThread::fileExtractDir(const bool &isSync)
{
    //! 解压缩文件到指定目录
    m_fileInfo.append(m_fileCompressed);
    m_totalSize = getUncompressedSize(m_fileCompressed);
    auto info = extractDir(m_fileCompressed, m_extractDir);
    m_run = false;
    if(isSync)
        return info;
    Q_EMIT signalExtractDirFinish(info);
}

QStringList zipCompressThread::fileExtractFiles(const bool &isSync)
{
    //!解压压缩文件中的指定文件到指定目录
    m_fileInfo = m_extractFiles;
    m_totalSize =  getUncompressedSize(m_fileCompressed,m_extractFiles);
    auto info = extractFiles(m_fileCompressed, m_extractFiles, m_extractDir);
    m_run = false;
    if(isSync)
        return info;
    Q_EMIT signalExtractFilesFinish(info);
}

qint64 zipCompressThread::calculateFolderSize(const QString& folderPath) {

    QDir directory(folderPath);
    qint64 totalSize = 0;

    if (!directory.exists()) {
        qWarning() << "Folder does not exist: " << folderPath;
        return -1;
    }
    //! 计算该目录中所有文件和文件夹的大小
    directory.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList fileList = directory.entryInfoList();

    foreach (const QFileInfo& fileInfo, fileList) {
        if (fileInfo.isFile()) {
            totalSize += fileInfo.size();
        } else if (fileInfo.isDir()) {
            qint64 subfolderSize = calculateFolderSize(fileInfo.filePath());
            if (subfolderSize >= 0) {
                totalSize += subfolderSize;
            } else {
                qWarning() << "Error calculating subfolder size: " << fileInfo.filePath();
            }
        }
    }
    return totalSize;
}
quint64 zipCompressThread::getUncompressedSize(const QString &zipFilePath, const QStringList &fileNames)
{
    QuaZip zip(zipFilePath);
    if (!zip.open(QuaZip::mdUnzip)) {
        qWarning() << "Failed to open zip file";
        return 0;
    }

    QuaZipFileInfo info;
    quint64 totalUncompressedSize = 0;

    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        if (!zip.getCurrentFileInfo(&info)) {
            qWarning() << "Failed to get file info";
            continue;
        }
        //! 获取压缩包里所有文件大小
        if(fileNames.isEmpty()){
            totalUncompressedSize += info.uncompressedSize;
        }else {
            //! 获取压缩包里指定文件大小
            if ( fileNames.contains(info.name)) {
                totalUncompressedSize += info.uncompressedSize;
            }
        }
    }
    zip.close();
    return totalUncompressedSize;
}

bool zipCompressThread::checkOperation(ZipCompressType type, QString fileCompressed)
{
    if (m_run) {
        qWarning() << "Please wait while executing:" << type;
        return false;
    }
    m_run = true;
    m_totalBytesRead = 0;
    m_totalSize = 0;
    m_compressType = type;
    m_fileCompressed = fileCompressed;
    m_fileInfo.clear();
    return true;
}

bool zipCompressThread::setupCompressOperation(ZipCompressType type, bool isSync)
{
    if (isSync) {
        if (m_compressType == CompressFiles || m_compressType == CompressDir) {
            return type == CompressFiles ? fileCompressFiles(isSync) : fileCompressDir(isSync);
        }else{
            qWarning() << " not found operation type";
        }
    }
    start();
    return true;
}

QStringList zipCompressThread::setupExtractOperation(ZipCompressType type, bool isSync)
{

    if (isSync) {
        if (type == ExtractFiles || type == ExtractDir) {
            return type == ExtractFiles ? fileExtractFiles(isSync) : fileExtractDir(isSync);
        } else {
            qWarning() << " not found operation type";
        }
    }
    start();
    return QStringList();
}
//TODO 后期考虑添加正在加解压文件名称输出
bool zipCompressThread::copyData(QIODevice &inFile, QIODevice &outFile,const QString  &fileName)
{
    qreal oldProgress = 0.0;
    QString type = enumToString(m_compressType);
    while (!inFile.atEnd()) {
        char buf[4096];
        qint64 readLen = inFile.read(buf, 4096);
        if (readLen <= 0)
            return false;
        if (outFile.write(buf, readLen) != readLen)
            return false;
        m_totalBytesRead += readLen;
        if (m_totalSize > 0) {
            qreal progress = static_cast<qreal>(m_totalBytesRead) / m_totalSize;
            QString result = QString::number(progress, 'f', 2);
            progress = result.toDouble();
            if(oldProgress != progress ){
                //WARNING 文本类型文件由于编码原因读取的大可能和文件占用大小不一致特殊处理进度不可能超过1
                if(progress > 1.00){
                    qWarning()<<" error schedule: "<< progress;
                    progress = 1.00;
                }
                Q_EMIT signalCompressionProgress(progress,type,m_fileInfo);
                oldProgress = progress;
            }
        }

    }

    return true;
}

qint64 zipCompressThread::calculateFilesSize(const QStringList &files)
{
    qint64 totalSize =-1;
    for(const auto & filePath:files) {
        QFileInfo fileInfo(filePath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            totalSize += fileInfo.size();
        } else {
            qWarning() << "File does not exist or is not a file: " << filePath;
        }
    }
    return totalSize;
}
bool zipCompressThread::compressFile(QuaZip* zip, QString fileName, QString fileDest) {
    // zip: oggetto dove aggiungere il file
    // fileName: nome del file reale
    // fileDest: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
            zip->getMode()!=QuaZip::mdAppend &&
            zip->getMode()!=QuaZip::mdAdd) return false;

    // Apro il file risulato
    QuaZipFile outFile(zip);
    if(!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, fileName))) return false;

    QFileInfo input(fileName);
    if (quazip_is_symlink(input)) {
        // Not sure if we should use any specialized codecs here.
        // After all, a symlink IS just a byte array. And
        // this is mostly for Linux, where UTF-8 is ubiquitous these days.
        QString path = quazip_symlink_target(input);
        QString relativePath = input.dir().relativeFilePath(path);
        outFile.write(QFile::encodeName(relativePath));
    } else {
        QFile inFile;
        inFile.setFileName(fileName);
        if (!inFile.open(QIODevice::ReadOnly))
            return false;
        if (!copyData(inFile, outFile,fileDest) || outFile.getZipError()!=UNZ_OK)
            return false;
        inFile.close();
    }

    // Chiudo i file
    outFile.close();
    return outFile.getZipError() == UNZ_OK;
}

bool zipCompressThread::compressSubDir(QuaZip* zip, QString dir, QString origDir, bool recursive, QDir::Filters filters) {
    // zip: oggetto dove aggiungere il file
    // dir: cartella reale corrente
    // origDir: cartella reale originale
    // (path(dir)-path(origDir)) = path interno all'oggetto zip

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdCreate &&
            zip->getMode()!=QuaZip::mdAppend &&
            zip->getMode()!=QuaZip::mdAdd) return false;

    // Controllo la cartella
    QDir directory(dir);
    if (!directory.exists()) return false;

    QDir origDirectory(origDir);
    if (dir != origDir) {
        QuaZipFile dirZipFile(zip);
        if (!dirZipFile.open(QIODevice::WriteOnly,
                             QuaZipNewInfo(origDirectory.relativeFilePath(dir) + QLatin1String("/"), dir), nullptr, 0, 0)) {
            return false;
        }
        dirZipFile.close();
    }


    // Se comprimo anche le sotto cartelle
    if (recursive) {
        // Per ogni sotto cartella
        QFileInfoList files = directory.entryInfoList(QDir::AllDirs|QDir::NoDotAndDotDot|filters);
        for (const auto& file : files) {
            if (!file.isDir()) // needed for Qt < 4.7 because it doesn't understand AllDirs
                continue;
            // Comprimo la sotto cartella
            if(!compressSubDir(zip,file.absoluteFilePath(),origDir,recursive,filters)) return false;
        }
    }

    // Per ogni file nella cartella
    QFileInfoList files = directory.entryInfoList(QDir::Files|filters);
    for (const auto& file : files) {
        // Se non e un file o e il file compresso che sto creando
        if(!file.isFile()||file.absoluteFilePath()==zip->getZipName()) continue;

        // Creo il nome relativo da usare all'interno del file compresso
        QString filename = origDirectory.relativeFilePath(file.absoluteFilePath());

        // Comprimo il file
        if (!compressFile(zip,file.absoluteFilePath(),filename)) return false;
    }

    return true;
}

bool zipCompressThread::extractFile(QuaZip* zip, QString fileName, QString fileDest) {
    // zip: oggetto dove aggiungere il file
    // filename: nome del file reale
    // fileincompress: nome del file all'interno del file compresso

    // Controllo l'apertura dello zip
    if (!zip) return false;
    if (zip->getMode()!=QuaZip::mdUnzip) return false;

    // Apro il file compresso
    if (!fileName.isEmpty())
        zip->setCurrentFile(fileName);
    QuaZipFile inFile(zip);
    if(!inFile.open(QIODevice::ReadOnly) || inFile.getZipError()!=UNZ_OK) return false;

    // Controllo esistenza cartella file risultato
    QDir curDir;
    if (fileDest.endsWith(QLatin1String("/"))) {
        if (!curDir.mkpath(fileDest)) {
            return false;
        }
    } else {
        if (!curDir.mkpath(QFileInfo(fileDest).absolutePath())) {
            return false;
        }
    }

    QuaZipFileInfo64 info;
    if (!zip->getCurrentFileInfo(&info))
        return false;

    QFile::Permissions srcPerm = info.getPermissions();
    if (fileDest.endsWith(QLatin1String("/")) && QFileInfo(fileDest).isDir()) {
        if (srcPerm != 0) {
            QFile(fileDest).setPermissions(srcPerm);
        }
        return true;
    }

    if (info.isSymbolicLink()) {
        QString target = QFile::decodeName(inFile.readAll());
        return QFile::link(target, fileDest);
    }

    // Apro il file risultato
    QFile outFile;
    outFile.setFileName(fileDest);
    if(!outFile.open(QIODevice::WriteOnly)) return false;

    // Copio i dati
    if (!copyData(inFile, outFile,fileDest) || inFile.getZipError()!=UNZ_OK) {
        outFile.close();
        removeFile(QStringList(fileDest));
        return false;
    }
    outFile.close();

    // Chiudo i file
    inFile.close();
    if (inFile.getZipError()!=UNZ_OK) {
        removeFile(QStringList(fileDest));
        return false;
    }

    if (srcPerm != 0) {
        outFile.setPermissions(srcPerm);
    }
    return true;
}

bool zipCompressThread::removeFile(QStringList listFile) {
    bool ret = true;
    // Per ogni file
    for (int i=0; i<listFile.count(); i++) {
        // Lo elimino
        ret = ret && QFile::remove(listFile.at(i));
    }
    return ret;
}

bool zipCompressThread::compressFile(QString fileCompressed, QString file) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo il file
    if (!compressFile(&zip,file,QFileInfo(file).fileName())) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool zipCompressThread::compressFiles(QString fileCompressed, QStringList files) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Comprimo i file
    QFileInfo info;
    for (int index = 0; index < files.size(); ++index ) {
        const QString & file( files.at( index ) );
        info.setFile(file);
        if (!info.exists() || !compressFile(&zip,file,info.fileName())) {
            QFile::remove(fileCompressed);
            return false;
        }
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

bool zipCompressThread::compressDir(QString fileCompressed, QString dir, bool recursive) {
    return compressDir(fileCompressed, dir, recursive, QDir::Filters());
}

bool zipCompressThread::compressDir(QString fileCompressed, QString dir,
                                    bool recursive, QDir::Filters filters)
{
    // Creo lo zip
    QuaZip zip(fileCompressed);
    QDir().mkpath(QFileInfo(fileCompressed).absolutePath());
    if(!zip.open(QuaZip::mdCreate)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Aggiungo i file e le sotto cartelle
    if (!compressSubDir(&zip,dir,dir,recursive, filters)) {
        QFile::remove(fileCompressed);
        return false;
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        QFile::remove(fileCompressed);
        return false;
    }

    return true;
}

QString zipCompressThread::extractFile(QString fileCompressed, QString fileName, QString fileDest) {
    // Apro lo zip
    QuaZip zip(fileCompressed);
    return extractFile(zip, fileName, fileDest);
}

QString zipCompressThread::extractFile(QuaZip &zip, QString fileName, QString fileDest)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QString();
    }

    // Estraggo il file
    if (fileDest.isEmpty())
        fileDest = fileName;
    if (!extractFile(&zip,fileName,fileDest)) {
        return QString();
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(QStringList(fileDest));
        return QString();
    }
    return QFileInfo(fileDest).absoluteFilePath();
}

QStringList zipCompressThread::extractFiles(QString fileCompressed, QStringList files, QString dir) {
    // Creo lo zip
    QuaZip zip(fileCompressed);
    return extractFiles(zip, files, dir);
}

QStringList zipCompressThread::extractFiles(QuaZip &zip, const QStringList &files, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }

    // Estraggo i file
    QStringList extracted;
    for (int i=0; i<files.count(); i++) {
        QString absPath = QDir(dir).absoluteFilePath(files.at(i));
        if (!extractFile(&zip, files.at(i), absPath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absPath);
    }

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList zipCompressThread::extractDir(QString fileCompressed, QTextCodec* fileNameCodec, QString dir) {
    // Apro lo zip
    QuaZip zip(fileCompressed);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);
    return extractDir(zip, dir);
}

QStringList zipCompressThread::extractDir(QString fileCompressed, QString dir) {
    return extractDir(fileCompressed, nullptr, dir);
}

QStringList zipCompressThread::extractDir(QuaZip &zip, const QString &dir)
{
    if(!zip.open(QuaZip::mdUnzip)) {
        return QStringList();
    }
    QString cleanDir = QDir::cleanPath(dir);
    QDir directory(cleanDir);
    QString absCleanDir = directory.absolutePath();
    if (!absCleanDir.endsWith(QLatin1Char('/'))) // It only ends with / if it's the FS root.
        absCleanDir += QLatin1Char('/');
    QStringList extracted;
    if (!zip.goToFirstFile()) {
        return QStringList();
    }
    do {
        QString name = zip.getCurrentFileName();
        QString absFilePath = directory.absoluteFilePath(name);
        QString absCleanPath = QDir::cleanPath(absFilePath);
        if (!absCleanPath.startsWith(absCleanDir))
            continue;
        if (!extractFile(&zip, QLatin1String(""), absFilePath)) {
            removeFile(extracted);
            return QStringList();
        }
        extracted.append(absFilePath);
    } while (zip.goToNextFile());

    // Chiudo il file zip
    zip.close();
    if(zip.getZipError()!=0) {
        removeFile(extracted);
        return QStringList();
    }

    return extracted;
}

QStringList zipCompressThread::getFileList(QString fileCompressed) {
    // Apro lo zip
    QuaZip* zip = new QuaZip(QFileInfo(fileCompressed).absoluteFilePath());
    return getFileList(zip);
}

QStringList zipCompressThread::getFileList(QuaZip *zip)
{
    if(!zip->open(QuaZip::mdUnzip)) {
        delete zip;
        return QStringList();
    }

    // Estraggo i nomi dei file
    QStringList lst;
    QuaZipFileInfo64 info;
    for(bool more=zip->goToFirstFile(); more; more=zip->goToNextFile()) {
        if(!zip->getCurrentFileInfo(&info)) {
            delete zip;
            return QStringList();
        }
        lst << info.name;
        //info.name.toLocal8Bit().constData()
    }

    // Chiudo il file zip
    zip->close();
    if(zip->getZipError()!=0) {
        delete zip;
        return QStringList();
    }
    delete zip;
    return lst;
}

QStringList zipCompressThread::extractDir(QIODevice* ioDevice, QTextCodec* fileNameCodec, QString dir)
{
    QuaZip zip(ioDevice);
    if (fileNameCodec)
        zip.setFileNameCodec(fileNameCodec);
    return extractDir(zip, dir);
}

QStringList zipCompressThread::extractDir(QIODevice *ioDevice, QString dir)
{
    return extractDir(ioDevice, nullptr, dir);
}

QStringList zipCompressThread::getFileList(QIODevice *ioDevice)
{
    QuaZip *zip = new QuaZip(ioDevice);
    return getFileList(zip);
}

QString zipCompressThread::extractFile(QIODevice *ioDevice, QString fileName, QString fileDest)
{
    QuaZip zip(ioDevice);
    return extractFile(zip, fileName, fileDest);
}

QStringList zipCompressThread::extractFiles(QIODevice *ioDevice, QStringList files, QString dir)
{
    QuaZip zip(ioDevice);
    return extractFiles(zip, files, dir);
}

}
