#include "zipcompress_p.h"
#include <QDebug>
namespace compressionTool {

zipCompressPrivate::zipCompressPrivate(zipCompress *ptr):
    m_zipCompressThread(new zipCompressThread)
{
    q_ptr = ptr;
}
zipCompressPrivate::~zipCompressPrivate()
{

}
bool zipCompressPrivate::fileCompressFiles(QString fileCompressed, QStringList files, bool isSync)
{
    return m_zipCompressThread->fileCompressFiles(fileCompressed,files,isSync);
}

QStringList zipCompressPrivate::fileExtractDir(QString fileCompressed, QString dir, bool isSync)
{
    return m_zipCompressThread->fileExtractDir(fileCompressed,dir,isSync);
}

bool zipCompressPrivate::fileCompressDir(QString fileCompressed, QString dir, bool isSync, bool recursive)
{
    return m_zipCompressThread->fileCompressDir(fileCompressed,dir,isSync,recursive);
}

QStringList zipCompressPrivate::fileExtractFiles(QString fileCompressed, QStringList files, QString dir, bool isSync)
{
    return m_zipCompressThread->fileExtractFiles(fileCompressed,files,dir,isSync);
}

void zipCompressPrivate::initConnect()
{
    Q_Q(zipCompress);
    QObject::connect(m_zipCompressThread.data(),&zipCompressThread::signalCompressionProgress,q,
                     &zipCompress::signalCompressionProgress);
    QObject::connect(m_zipCompressThread.data(),&zipCompressThread::signalCompressDirinish,q,
                     &zipCompress::signalCompressDirinish);
    QObject::connect(m_zipCompressThread.data(),&zipCompressThread::signalCompressFilesFinish,q,
                     &zipCompress::signalCompressFilesFinish);
    QObject::connect(m_zipCompressThread.data(),&zipCompressThread::signalExtractFilesFinish,q,
                     &zipCompress::signalExtractFilesFinish);
    QObject::connect(m_zipCompressThread.data(),&zipCompressThread::signalExtractDirFinish,q,
                     &zipCompress::signalExtractDirFinish);


}

QStringList zipCompressPrivate::getFileList(QString fileCompressed)
{
    return m_zipCompressThread->getFileList(fileCompressed);
}
}
