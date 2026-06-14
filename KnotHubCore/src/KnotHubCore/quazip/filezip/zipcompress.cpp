#include "zipcompress.h"
#include "zipcompress_p.h"
namespace compressionTool {
zipCompress::zipCompress(QObject *parent):
    QObject(*new zipCompressPrivate(this),parent)
{
    Q_D(zipCompress);
    d->initConnect();
}
zipCompress::~zipCompress()
{

}

QStringList zipCompress::getFileList(QString fileCompressed)
{
    Q_D(zipCompress);
    return  d->getFileList(fileCompressed);
}
bool zipCompress::syncCompressFiles(QString fileCompressed, QStringList files)
{
    Q_D(zipCompress);
    return  d->fileCompressFiles(fileCompressed,files,true);
}

void zipCompress::asyncCompressFiles(QString fileCompressed, QStringList files)
{
    Q_D(zipCompress);
    d->fileCompressFiles(fileCompressed,files,false);
}

QStringList zipCompress::syncExtractDir(QString fileCompressed, QString dir)
{
    Q_D(zipCompress);
    return  d->fileExtractDir(fileCompressed,dir,true);
}

void zipCompress::asyncExtractDir(QString fileCompressed, QString dir)
{
    Q_D(zipCompress);
    d->fileExtractDir(fileCompressed,dir,false);
}

bool zipCompress::syncCompressDir(QString fileCompressed, QString dir, bool recursive)
{
    Q_D(zipCompress);
    return  d->fileCompressDir(fileCompressed,dir,true,recursive);
}

void zipCompress::asyncCompressDir(QString fileCompressed, QString dir, bool recursive)
{
    Q_D(zipCompress);
    d->fileCompressDir(fileCompressed,dir,false,recursive);
}

QStringList zipCompress::syncExtractFiles(QString fileCompressed, QStringList files, QString dir)
{
    Q_D(zipCompress);
    return  d->fileExtractFiles(fileCompressed,files,dir,true);
}

void zipCompress::asyncExtractFiles(QString fileCompressed, QStringList files, QString dir)
{
    Q_D(zipCompress);
    d->fileExtractFiles(fileCompressed,files,dir,false);
}

}
