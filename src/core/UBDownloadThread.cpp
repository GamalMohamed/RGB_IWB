#include <QDebug>
#include <QNetworkProxy>
#include <QNetworkDiskCache>

#include "core/UBSettings.h"

#include "UBDownloadThread.h"

#include "core/memcheck.h"

/**
 * \brief Constructor
 * @param parent as the parent object
 * @param name as the object name
 */
UBDownloadThread::UBDownloadThread(QObject *parent, const char *name):QThread(parent)
    , mbRun(false)
    ,mpReply(NULL)
{
    setObjectName(name);
}

/**
 * \brief Destructor
 */
UBDownloadThread::~UBDownloadThread()
{
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

/**
 * \brief Run the thread
 */
void UBDownloadThread::run()
{
    qDebug() << mUrl;
    // We start the download
    QNetworkAccessManager* pNam = new QNetworkAccessManager();

    mpReply = pNam->get(QNetworkRequest(QUrl(mUrl)));
    qDebug() << " -- Http GET reply ---------------------- ";
    qDebug() << mpReply->readAll();
    qDebug() << " ---------------------------------------- ";

    connect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));

    while(mbRun)
    {
        // Wait here until the end of the download
        sleep(100);
    }

    disconnect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    disconnect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

/**
 * \brief Stop the current download
 */
void UBDownloadThread::stopDownload()
{
    mbRun = false;
}

/**
 * \brief Start the download
 */
void UBDownloadThread::startDownload(int id, QString url)
{
    mID = id;
    mUrl = url;
    mbRun = true;
    start();
}

/**
 * \brief Notify the download progression
 * @param received as the number of bytes received
 * @param total as the total number of bytes of the file
 */
void UBDownloadThread::onDownloadProgress(qint64 received, qint64 total)
{
    qDebug() << received << " on " << total;
    emit downloadProgress(mID, received, total);
}

/**
 * \brief Notify the end of the download
 */
void UBDownloadThread::onDownloadFinished()
{
    emit downloadFinised(mID);
}
