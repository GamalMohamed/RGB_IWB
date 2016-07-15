/*
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of Open-Sankoré.
 *
 * Open-Sankoré is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * Open-Sankoré is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Open-Sankoré.  If not, see <http://www.gnu.org/licenses/>.
 */



#include "UBWidgetUniboardAPI.h"

#include <QWebView>
#include <QDomDocument>
#include <QtGui>

#include "gui/UBMainWindow.h"

#include "core/UB.h"
#include "core/UBApplication.h"
#include "core/UBSettings.h"

#include "document/UBDocumentProxy.h"

#include "board/UBBoardController.h"
#include "board/UBDrawingController.h"
#include "board/UBBoardPaletteManager.h"

#include "domain/UBGraphicsScene.h"
#include "domain/UBGraphicsWidgetItem.h"

#include "adaptors/UBThumbnailAdaptor.h"

#include "UBWidgetMessageAPI.h"
#include "frameworks/UBFileSystemUtils.h"
#include "core/UBDownloadManager.h"

#include "core/memcheck.h"

//Known extentions for files, add if you know more supported
const QString audioExtentions = ".mp3.wma.ogg";
const QString videoExtentions = ".avi.flv";
const QString imageExtentions = ".png.jpg.tif.bmp.tga";
const QString htmlExtentions = ".htm.html.xhtml";

//Allways use aliases instead of const char* itself
const QString imageAlias    = "image";
const QString imageAliasCap = "Image";
const QString videoAlias    = "video";
const QString videoAliasCap = "Video";
const QString audioAlias    = "audio";
const QString audioAliasCap = "Audio";

//Xml tag names
const QString tMainSection = "mimedata";
const QString tType = "type";
const QString tPath = "path";
const QString tMessage = "message";
const QString tReady = "ready";

const QString tMimeText = "text/plain";


//Name of path inside widget to store objects
const QString objectsPath = "objects";

UBWidgetUniboardAPI::UBWidgetUniboardAPI(UBGraphicsScene *pScene, UBGraphicsWidgetItem *widget)
    : QObject(pScene)
    , mScene(pScene)
    , mGraphicsWidget(widget)
    , mIsVisible(false)
    , mMessagesAPI(0)
    , mDatastoreAPI(0)
 {
    UBGraphicsW3CWidgetItem* w3CGraphicsWidget = dynamic_cast<UBGraphicsW3CWidgetItem*>(widget);

    if (w3CGraphicsWidget)
    {
        mMessagesAPI = new UBWidgetMessageAPI(w3CGraphicsWidget);
        mDatastoreAPI = new UBDatastoreAPI(w3CGraphicsWidget);
    }

    connect(UBDownloadManager::downloadManager(), SIGNAL(downloadFinished(bool,sDownloadFileDesc,QByteArray)), this, SLOT(onDownloadFinished(bool,sDownloadFileDesc,QByteArray)));
}


UBWidgetUniboardAPI::~UBWidgetUniboardAPI()
{
    // NOOP
}

QObject* UBWidgetUniboardAPI::messages()
{
    return mMessagesAPI;
}


QObject* UBWidgetUniboardAPI::datastore()
{
    return mDatastoreAPI;
}


void UBWidgetUniboardAPI::setTool(const QString& toolString)
{
    if (UBApplication::boardController->activeScene() != mScene)
        return;

    const QString lower = toolString.toLower();

    if (lower == "pen")
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Pen);
    }
    else if (lower == "marker")
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Marker);
    }
    else if (lower == "arrow")
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);
    }
    else if (lower == "play")
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Play);
    }
    else if (lower == "line")
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Line);
    }
}


void UBWidgetUniboardAPI::setPenColor(const QString& penColor)
{
    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBSettings* settings = UBSettings::settings();

    bool conversionState = false;

    int index = penColor.toInt(&conversionState) - 1;

    if (conversionState && index > 0 && index <= 4)
    {
        UBApplication::boardController->setPenColorOnDarkBackground(settings->penColors(true).at(index - 1));
        UBApplication::boardController->setPenColorOnLightBackground(settings->penColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor.setNamedColor(penColor);
        if (svgColor.isValid())
        {
            UBApplication::boardController->setPenColorOnDarkBackground(svgColor);
            UBApplication::boardController->setPenColorOnLightBackground(svgColor);
        }
    }
}

void UBWidgetUniboardAPI::updateFontFamilyPreference(const QString& fontFamily)
{
    UBSettings::settings()->setFontFamily(fontFamily);
}

void UBWidgetUniboardAPI::updateFontSizePreference(const QString& fontSize)
{
    UBSettings::settings()->setFontPointSize(fontSize.toInt());
}

void UBWidgetUniboardAPI::updateFontBoldPreference()
{
    UBSettings::settings()->setBoldFont(!fontBoldPreference());
}

void UBWidgetUniboardAPI::updateFontItalicPreference()
{
    UBSettings::settings()->setItalicFont(!fontItalicPreference());
}

QString UBWidgetUniboardAPI::fontFamilyPreference()
{
    return UBSettings::settings()->fontFamily();
}

QString UBWidgetUniboardAPI::fontSizePreference()
{
    return QString::number(UBSettings::settings()->fontPointSize());
}

bool UBWidgetUniboardAPI::fontBoldPreference()
{
    return UBSettings::settings()->isBoldFont();
}

bool UBWidgetUniboardAPI::fontItalicPreference()
{
    return UBSettings::settings()->isItalicFont();
}

bool UBWidgetUniboardAPI::isDarkBackground()
{
    return UBApplication::boardController->activeScene()->isDarkBackground();
}

void UBWidgetUniboardAPI::setMarkerColor(const QString& penColor)
{
    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBSettings* settings = UBSettings::settings();

    bool conversionState = false;

    int index = penColor.toInt(&conversionState);

    if (conversionState && index > 0 && index <= 4)
    {
        UBApplication::boardController->setMarkerColorOnDarkBackground(settings->markerColors(true).at(index - 1));
        UBApplication::boardController->setMarkerColorOnLightBackground(settings->markerColors(false).at(index - 1));
    }
    else
    {
        QColor svgColor;
        svgColor.setNamedColor(penColor);
        if (svgColor.isValid())
        {
            UBApplication::boardController->setMarkerColorOnDarkBackground(svgColor);
            UBApplication::boardController->setMarkerColorOnLightBackground(svgColor);
        }
    }
}


void UBWidgetUniboardAPI::addObject(QString pUrl, int width, int height, int x, int y, bool background)
{
    // not great, but make it easily scriptable --
    //
    // download url should be moved to the scene from the controller
    //

    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBApplication::boardController->downloadURL(QUrl(pUrl), QString(), QPointF(x, y), QSize(width, height), background);

}


void UBWidgetUniboardAPI::setBackground(bool pIsDark, bool pIsCrossed)
{
    if (mScene)
        mScene->setBackground(pIsDark, pIsCrossed);
}


void UBWidgetUniboardAPI::moveTo(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (mScene)
    mScene->moveTo(QPointF(x, y));
}


void UBWidgetUniboardAPI::drawLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth)
        || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
        return;

    if (mScene)
    mScene->drawLineTo(QPointF(x, y), pWidth,
        UBDrawingController::drawingController()->stylusTool() == UBStylusTool::Line);
}


void UBWidgetUniboardAPI::eraseLineTo(const qreal x, const qreal y, const qreal pWidth)
{
    if (qIsNaN(x) || qIsNaN(y) || qIsNaN(pWidth)
       || qIsInf(x) || qIsInf(y) || qIsInf(pWidth))
       return;

    if (mScene)
    mScene->eraseLineTo(QPointF(x, y), pWidth);
}


void UBWidgetUniboardAPI::clear()
{
    if (mScene)
            mScene->clearContent(UBGraphicsScene::clearItemsAndAnnotations);
}


void UBWidgetUniboardAPI::zoom(const qreal factor, const qreal x, const qreal y)
{
   if (qIsNaN(factor) || qIsNaN(x) || qIsNaN(y)
       || qIsInf(factor) || qIsInf(x) || qIsInf(y))
       return;


    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBApplication::boardController->zoom(factor, QPointF(x, y));
}


void UBWidgetUniboardAPI::centerOn(const qreal x, const qreal y)
{
   if (qIsNaN(x) || qIsNaN(y)
       || qIsInf(x) || qIsInf(y))
       return;

    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBApplication::boardController->centerOn(QPointF(x, y));
}


void UBWidgetUniboardAPI::move(const qreal x, const qreal y)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (UBApplication::boardController->activeScene() != mScene)
        return;

    UBApplication::boardController->handScroll(x, y);
}


void UBWidgetUniboardAPI::addText(const QString& text, const qreal x, const qreal y, const int size, const QString& font
        , bool bold, bool italic)
{
    if (qIsNaN(x) || qIsNaN(y)
        || qIsInf(x) || qIsInf(y))
        return;

    if (UBApplication::boardController->activeScene() != mScene)
        return;

    if (mScene)
        mScene->addTextWithFont(text, QPointF(x, y), size, font, bold, italic);

}


int UBWidgetUniboardAPI::pageCount()
{
    if (mScene && mScene->document())
        return mScene->document()->pageCount();
    else
        return -1;
}


int UBWidgetUniboardAPI::currentPageNumber()
{
    // TODO UB 4.x widget find a better way to get the current page number

    if (UBApplication::boardController->activeScene() != mScene)
        return -1;

    return UBApplication::boardController->activeSceneIndex() + 1;
}

QString UBWidgetUniboardAPI::getObjDir()
{
    return mGraphicsWidget->getOwnFolder().toLocalFile() + "/" + objectsPath + "/";
}

void UBWidgetUniboardAPI::showMessage(const QString& message)
{
    UBApplication::boardController->showMessage(message, false);
}

void UBWidgetUniboardAPI::loadUrl(const QString& url)
{
    UBApplication::loadUrl(url);
}

void UBWidgetUniboardAPI::connectionError(const QString& message)
{
    UBApplication::mainWindow->warning(tr("Warning"),
                                       tr("Impossible to connect to Planete Sankore: %1").arg(message));
}

bool UBWidgetUniboardAPI::currentToolIsSelector()
{
    return ((UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool() == UBStylusTool::Selector);
}

bool UBWidgetUniboardAPI::isSelected()
{
    return mGraphicsWidget->isSelected();
}

QString UBWidgetUniboardAPI::widgetScale()
{
    // Issue - ALTI/AOU - 20140414 : RichTextEditor and NormalTextEditor were not displaying text at the same size (for a same Font Size).
    // The reason is when a W3CWidgetItem is put on the Board, it has a scale inverse to Board scale.
    // But NormalTextEditor has a scale factor of 1. In order to have a similar display, we set the same 1 scale factor for RichTextEditor :
    return QString::number(1.0 / mGraphicsWidget->transform().m22());
}

QString UBWidgetUniboardAPI::pageThumbnail(const int pageNumber)
{
    if (UBApplication::boardController->activeScene() != mScene)
        return "";

    UBDocumentProxy *doc = UBApplication::boardController->selectedDocument();

    if (!doc)
        return "";

    if (pageNumber > doc->pageCount())
        return "";

    QUrl url = UBThumbnailAdaptor::thumbnailUrl(doc, pageNumber - 1);

    return url.toString();

}


void UBWidgetUniboardAPI::resize(qreal width, qreal height)
{
    if (qIsNaN(width) || qIsNaN(height)
        || qIsInf(width) || qIsInf(height))
        return;

    if (mGraphicsWidget)
    {
        mGraphicsWidget->resize(width, height);
    }
}


void UBWidgetUniboardAPI::setPreference(const QString& key, QString value)
{
    if (mGraphicsWidget)
    {
            mGraphicsWidget->setPreference(key, value);
    }
}


QString UBWidgetUniboardAPI::preference(const QString& key , const QString& pDefault)
{
    if (mGraphicsWidget && mGraphicsWidget->preferences().contains(key))
    {
        return mGraphicsWidget->preference(key);
    }
    else
    {
        return pDefault;
    }
}


QStringList UBWidgetUniboardAPI::preferenceKeys()
{
    QStringList keys;

    if (mGraphicsWidget)
        keys = mGraphicsWidget->preferences().keys();

    return keys;
}


QString UBWidgetUniboardAPI::uuid()
{
    if (mGraphicsWidget)
        return UBStringUtils::toCanonicalUuid(mGraphicsWidget->uuid());
    else
        return "";
}


QString UBWidgetUniboardAPI::locale()
{
    return QLocale().name();
}

QString UBWidgetUniboardAPI::lang()
{
    QString lang = QLocale().name();

    if (lang.length() > 2)
        lang[2] = QLatin1Char('-');

    return lang;
}

void UBWidgetUniboardAPI::returnStatus(const QString& method, const QString& status)
{
    QString msg = QString(tr("%0 called (method=%1, status=%2)")).arg("returnStatus").arg(method).arg(status);
    UBApplication::showMessage(msg);
}

void UBWidgetUniboardAPI::usedMethods(QStringList methods)
{
    // TODO: Implement this method
    foreach(QString method, methods)
    {

    }
}

void UBWidgetUniboardAPI::response(bool correct)
{
    Q_UNUSED(correct);
    // TODO: Implement this method
}

void UBWidgetUniboardAPI::sendFileMetadata(QString metaData)
{
    //  Build the QMap of metadata and send it to application
    QMap<QString, QString> qmMetaDatas;
    QDomDocument domDoc;
    domDoc.setContent(metaData);
    QDomElement rootElem = domDoc.documentElement();
    QDomNodeList children = rootElem.childNodes();
    for(int i=0; i<children.size(); i++){
        QDomNode dataNode = children.at(i);
        QDomElement keyElem = dataNode.firstChildElement("key");
        QDomElement valueElem = dataNode.firstChildElement("value");
        qmMetaDatas[keyElem.text()] = valueElem.text();
    }

    UBApplication::boardController->displayMetaData(qmMetaDatas);
}

void UBWidgetUniboardAPI::enableDropOnWidget(bool enable)
{
    if (mGraphicsWidget)
    {
        mGraphicsWidget->setAcceptDrops(enable);
    }
}

void UBWidgetUniboardAPI::ProcessDropEvent(QGraphicsSceneDragDropEvent *event)
{
    const QMimeData *pMimeData = event->mimeData();

    QString destFileName;
    QString contentType;
    bool downloaded = false;

    QGraphicsView *tmpView = mGraphicsWidget->scene()->views().at(0);
    QPoint dropPoint(mGraphicsWidget->mapFromScene(tmpView->mapToScene(event->pos().toPoint())).toPoint());
    Qt::DropActions dropActions = event->possibleActions();
    Qt::MouseButtons dropMouseButtons = event->buttons();
    Qt::KeyboardModifiers dropModifiers = event->modifiers();
    QMimeData *dropMimeData = new QMimeData;
    qDebug() << event->possibleActions();


    if (pMimeData->hasHtml()) { //Dropping element from web browser
        QString qsHtml = pMimeData->html();
        QString url = UBApplication::urlFromHtml(qsHtml);

        if(!url.isEmpty()) {
            QString str = "test string";

            QMimeData mimeData;
            mimeData.setData(tMimeText, str.toAscii());

            sDownloadFileDesc desc;
            desc.dest = sDownloadFileDesc::graphicsWidget;
            desc.modal = true;
            desc.srcUrl = url;
            desc.currentSize = 0;
            desc.name = QFileInfo(url).fileName();
            desc.totalSize = 0; // The total size will be retrieved during the download

            desc.dropPoint = event->pos().toPoint(); //Passing pure event point. No modifications
            desc.dropActions = dropActions;
            desc.dropMouseButtons = dropMouseButtons;
            desc.dropModifiers = dropModifiers;

            registerIDWidget(UBDownloadManager::downloadManager()->addFileToDownload(desc));

        }

    } else  if (pMimeData->hasUrls()) { //Local file processing
        QUrl curUrl = pMimeData->urls().first();
        QString sUrl = curUrl.toString();

        if (sUrl.startsWith("file://") || sUrl.startsWith("/")) {
            QString fileName = curUrl.toLocalFile();
            QString extention = UBFileSystemUtils::extension(fileName);
            contentType = UBFileSystemUtils::mimeTypeFromFileName(fileName);

            if (supportedTypeHeader(contentType)) {
                destFileName = getObjDir() + QUuid::createUuid().toString() + "." + extention;

                if (!UBFileSystemUtils::copyFile(fileName, destFileName)) {
                    qDebug() << "can't copy from" << fileName << "to" << destFileName;
                    return;
                }
                downloaded = true;

            }
        }
    }

    QString mimeText = createMimeText(downloaded, contentType, destFileName);

    // Ev-5.1 - CFA - 20140109 : correction drop RTE
    UBFeaturesController* c = UBApplication::boardController->paletteManager()->featuresWidget()->getFeaturesController();
    if (c->getFeatureByFullPath(mGraphicsWidget->sourceUrl().toLocalFile()).getType() != FEATURE_RTE)
        dropMimeData->setData(tMimeText, mimeText.toAscii());

    if (mGraphicsWidget->page() && mGraphicsWidget->page()->mainFrame()) {
        mGraphicsWidget
            ->page()
            ->mainFrame()
            ->evaluateJavaScript("if(widget && widget.ondrop) { widget.ondrop('" + destFileName + "', '" + contentType + "');}");
    }

    event->setMimeData(dropMimeData);
}

void UBWidgetUniboardAPI::onDownloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData)
{
    //if widget recieves is waiting for this id then process
    if (!takeIDWidget(desc.id))
        return;

    if (!pSuccess) {
        qDebug() << "can't download the whole data. An error occured";
        return;
    }

    QString contentType = desc.contentTypeHeader;
    QString extention = UBFileSystemUtils::fileExtensionFromMimeType(contentType);

    if (!supportedTypeHeader(contentType)) {
        qDebug() << "actions for mime type" << contentType << "are not supported";
        return;
    }

    QString objDir = getObjDir();
    if (!QDir().exists(objDir)) {
        if (!QDir().mkpath(objDir)) {
            qDebug() << "can't create objects directory path. Check the permissions";
            return;
        }
    }

    QString destFileName = objDir + QUuid::createUuid() + "." + extention;
    QFile destFile(destFileName);

    if (!destFile.open(QIODevice::WriteOnly)) {
        qDebug() << "can't open" << destFileName << "for wrighting";
        return;
    }

    if (destFile.write(pData) == -1) {
        qDebug() << "can't implement data writing";
        return;
    }

    QGraphicsView *tmpView = mGraphicsWidget->scene()->views().at(0);
    QPoint dropPoint(mGraphicsWidget->mapFromScene(tmpView->mapToScene(desc.dropPoint)).toPoint());

    QMimeData dropMimeData;
    QString mimeText = createMimeText(true, contentType, destFileName);
    dropMimeData.setData(tMimeText, mimeText.toAscii());

    destFile.close();

    //To make js interpreter accept drop event we need to generate move event first.
    QDragMoveEvent pseudoMove(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    QApplication::sendEvent(mGraphicsWidget,&pseudoMove);

    QDropEvent readyEvent(dropPoint, desc.dropActions, &dropMimeData, desc.dropMouseButtons, desc.dropModifiers);
    //sending event to destination either it had been downloaded or not
    QApplication::sendEvent(mGraphicsWidget,&readyEvent);
    readyEvent.acceptProposedAction();
}

QString UBWidgetUniboardAPI::createMimeText(bool downloaded, const QString &mimeType, const QString &fileName)
{
    QString mimeXml;
    QXmlStreamWriter writer(&mimeXml);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement(tMainSection);

    writer.writeTextElement(tReady, boolToStr(downloaded));

    if (downloaded) {
        if (!mimeType.isEmpty()) {
            writer.writeTextElement(tType, mimeType);  //writing type of element
        }
        if (!QFile::exists(fileName)) {
            qDebug() << "file" << fileName << "doesn't exist";
            return QString();
        }

        QString relatedFileName = fileName;
        relatedFileName = relatedFileName.remove(mGraphicsWidget->getOwnFolder().toLocalFile() + "/");
        writer.writeTextElement(tPath, relatedFileName);   //writing path to created object
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    return mimeXml;
}

bool UBWidgetUniboardAPI::supportedTypeHeader(const QString &typeHeader) const
{
    return     typeHeader.startsWith(imageAlias) || typeHeader.startsWith(imageAliasCap)
            || typeHeader.startsWith(audioAlias) || typeHeader.startsWith(audioAliasCap)
            || typeHeader.startsWith(videoAlias) || typeHeader.startsWith(videoAliasCap);
}

bool UBWidgetUniboardAPI::takeIDWidget(int id)
{
    if (webDownloadIds.contains(id)) {
        webDownloadIds.removeAll(id);
        return true;
    }
    return false;
}

bool UBWidgetUniboardAPI::isDropableData(const QMimeData *pMimeData) const
{
    QString fileName = QString();
    if (pMimeData->hasHtml()) {
        fileName = UBApplication::urlFromHtml(pMimeData->html());
        if (fileName.isEmpty())
            return false;
    } else if (pMimeData->hasUrls()) {
        fileName = pMimeData->urls().at(0).toLocalFile();
        if (fileName.isEmpty())
            return false;
    }

    if (supportedTypeHeader(UBFileSystemUtils::mimeTypeFromFileName(fileName)))
        return true;

    return false;
}


bool UBWidgetUniboardAPI::removeFile(const QString &path)
{
    QString url =  mGraphicsWidget->url().toLocalFile();

    QString wgtUrl;
    bool hasFoundWgtExtention = false;

    //find the path of the widget
    foreach(QString fragment, url.split('/')){
        if(!hasFoundWgtExtention){
            wgtUrl += fragment + '/';

            hasFoundWgtExtention  = fragment.contains(".wgt");
        }
    }

    //then find the absolute path of the ressource
    foreach(QString fragment, path.split('/')){
        if(fragment != ".." && fragment != "."){
            wgtUrl += fragment + '/';
        }
    }

    QFile file(wgtUrl.mid(0, wgtUrl.size()-1));

    return file.remove();
}


UBDocumentDatastoreAPI::UBDocumentDatastoreAPI(UBGraphicsW3CWidgetItem *graphicsWidget)
    : UBW3CWebStorage(graphicsWidget)
    , mGraphicsW3CWidget(graphicsWidget)
{
    // NOOP
}



UBDocumentDatastoreAPI::~UBDocumentDatastoreAPI()
{
    // NOOP
}


QString UBDocumentDatastoreAPI::key(int index)
{
   QMap<QString, QString> entries = mGraphicsW3CWidget->datastoreEntries();

   if (index < entries.size())
       return entries.keys().at(index);
   else
       return "";

}


QString UBDocumentDatastoreAPI::getItem(const QString& key)
{
    QMap<QString, QString> entries = mGraphicsW3CWidget->datastoreEntries();
    if (entries.contains(key))
    {
        return entries.value(key);
    }
    else
    {
        return "";
    }
}


int UBDocumentDatastoreAPI::length()
{
   return mGraphicsW3CWidget->datastoreEntries().size();
}


void UBDocumentDatastoreAPI::setItem(const QString& key, const QString& value)
{
    if (mGraphicsW3CWidget)
    {
        mGraphicsW3CWidget->setDatastoreEntry(key, value);
    }
}


void UBDocumentDatastoreAPI::removeItem(const QString& key)
{
    mGraphicsW3CWidget->removeDatastoreEntry(key);
}
void

 UBDocumentDatastoreAPI::clear()
{
    mGraphicsW3CWidget->removeAllDatastoreEntries();
}


UBDatastoreAPI::UBDatastoreAPI(UBGraphicsW3CWidgetItem *widget)
    : QObject(widget)
{
    mDocumentDatastore = new UBDocumentDatastoreAPI(widget);
}


QObject* UBDatastoreAPI::document()
{
    return mDocumentDatastore;
}



