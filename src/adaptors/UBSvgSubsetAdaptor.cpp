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



#include "UBSvgSubsetAdaptor.h"

#include <QtCore>
#include <QtXml>

#include "domain/UBGraphicsSvgItem.h"
#include "domain/UBGraphicsPixmapItem.h"
#include "domain/UBGraphicsProxyWidget.h"
#include "domain/UBGraphicsPolygonItem.h"
#include "domain/UBGraphicsMediaItem.h"
#include "domain/UBGraphicsWidgetItem.h"
#include "domain/UBGraphicsPDFItem.h"
#include "domain/UBGraphicsTextItem.h"
#include "domain/UBGraphicsTextItemDelegate.h"
#include "domain/UBGraphicsStroke.h"
#include "domain/UBGraphicsStrokesGroup.h"
#include "domain/UBGraphicsGroupContainerItem.h"
#include "domain/UBGraphicsGroupContainerItemDelegate.h"
#include "domain/UBGraphicsEllipseItem.h"
#include "domain/UBGraphicsRectItem.h"
#include "domain/UBEditableGraphicsPolygonItem.h"
#include "domain/UBGraphicsFreehandItem.h"
#include "domain/UBEditableGraphicsRegularShapeItem.h"
#include "domain/UBGraphicsLineItem.h"
#include "domain/UB1HEditableGraphicsSquareItem.h"
#include "domain/UB1HEditableGraphicsCircleItem.h"

#include "domain/UBItem.h"

#include "tools/UBGraphicsRuler.h"
#include "tools/UBGraphicsCompass.h"
#include "tools/UBGraphicsProtractor.h"
#include "tools/UBGraphicsCurtainItem.h"
#include "tools/UBGraphicsTriangle.h"
#include "tools/UBGraphicsCache.h"

#include "document/UBDocumentProxy.h"

#include "board/UBBoardView.h"
#include "board/UBBoardController.h"
#include "board/UBDrawingController.h"
#include "board/UBBoardPaletteManager.h"

#include "frameworks/UBFileSystemUtils.h"
#include "frameworks/UBStringUtils.h"
#include "frameworks/UBFileSystemUtils.h"

#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBPersistenceManager.h"
#include "core/UBApplication.h"
#include "core/UBTextTools.h"
#include "gui/UBDrawingStrokePropertiesPalette.h"

#include "gui/UBTeacherGuideWidget.h"
#include "gui/UBDockTeacherGuideWidget.h"

#include "interfaces/IDataStorage.h"

#include "document/UBDocumentContainer.h"

#include "pdf/PDFRenderer.h"

#include "customWidgets/UBGraphicsItemAction.h"

#include "core/memcheck.h"
//#include "qtlogger.h"

const QString UBSvgSubsetAdaptor::nsSvg = "http://www.w3.org/2000/svg";
const QString UBSvgSubsetAdaptor::nsXHtml = "http://www.w3.org/1999/xhtml";
const QString UBSvgSubsetAdaptor::nsXLink = "http://www.w3.org/1999/xlink";
const QString UBSvgSubsetAdaptor::xmlTrue = "true";
const QString UBSvgSubsetAdaptor::xmlFalse = "false";
const QString UBSvgSubsetAdaptor::sFontSizePrefix = "font-size:";
const QString UBSvgSubsetAdaptor::sPixelUnit = "px";
const QString UBSvgSubsetAdaptor::sFontWeightPrefix = "font-weight:";
const QString UBSvgSubsetAdaptor::sFontStylePrefix = "font-style:";
const QString UBSvgSubsetAdaptor::sFormerUniboardDocumentNamespaceUri = "http://www.mnemis.com/uniboard";

const QString UBSvgSubsetAdaptor::SVG_STROKE_DOTLINE = "20 10"; // 1 big dot, 1 little space

const QString tElement = "element";
const QString tGroup = "group";
const QString tStrokeGroup = "strokeGroup";
const QString tGroups = "groups";
const QString aId = "id";

static bool mIsOldVersionFileWithText = false;

QMap<QString,IDataStorage*> UBSvgSubsetAdaptor::additionalElementToStore;

QString UBSvgSubsetAdaptor::toSvgTransform(const QMatrix& matrix)
{
    return QString("matrix(%1, %2, %3, %4, %5, %6)")
           .arg(matrix.m11(), 0 , 'g')
           .arg(matrix.m12(), 0 , 'g')
           .arg(matrix.m21(), 0 , 'g')
           .arg(matrix.m22(), 0 , 'g')
           .arg(matrix.dx(), 0 , 'g')
           .arg(matrix.dy(), 0 , 'g');
}


QMatrix UBSvgSubsetAdaptor::fromSvgTransform(const QString& transform)
{
    QMatrix matrix;
    QString ts = transform;
    ts.replace("matrix(", "");
    ts.replace(")", "");
    QStringList sl = ts.split(",");

    if (sl.size() >= 6)
    {
        matrix.setMatrix(
            sl.at(0).toFloat(),
            sl.at(1).toFloat(),
            sl.at(2).toFloat(),
            sl.at(3).toFloat(),
            sl.at(4).toFloat(),
            sl.at(5).toFloat());
    }

    return matrix;
}



static bool itemZIndexComp(const QGraphicsItem* item1,
                           const QGraphicsItem* item2)
{
    return item1->data(UBGraphicsItemData::ItemOwnZValue).toReal() < item2->data(UBGraphicsItemData::ItemOwnZValue).toReal();
}


void UBSvgSubsetAdaptor::upgradeScene(UBDocumentProxy* proxy, const int pageIndex)
{
    //4.2
    QDomDocument doc = loadSceneDocument(proxy, pageIndex);
    QDomElement elSvg = doc.documentElement(); // SVG tag
    QString ubVersion = elSvg.attributeNS(UBSettings::uniboardDocumentNamespaceUri, "version", "4.1"); // default to 4.1

    if (ubVersion.startsWith("4.1") || ubVersion.startsWith("4.2") || ubVersion.startsWith("4.3"))
    {
        // migrate to 4.2.1 (or latter)
        UBGraphicsScene *scene = loadScene(proxy, pageIndex);
        scene->setModified(true);
        persistScene(proxy, scene, pageIndex);
    }
}


QDomDocument UBSvgSubsetAdaptor::loadSceneDocument(UBDocumentProxy* proxy, const int pPageIndex)
{
    QString fileName = proxy->persistencePath() + UBFileSystemUtils::digitFileFormat("/page%1.svg",pPageIndex);

    QFile file(fileName);
    QDomDocument doc("page");

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return doc;
        }

        doc.setContent(&file, true);
        file.close();
    }

    return doc;
}


void UBSvgSubsetAdaptor::setSceneUuid(UBDocumentProxy* proxy, const int pageIndex, QUuid pUuid)
{
    QString fileName = proxy->persistencePath() + UBFileSystemUtils::digitFileFormat("/page%1.svg",pageIndex);

    QFile file(fileName);

    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return;

    QTextStream textReadStream(&file);
    QString xmlContent = textReadStream.readAll();
    int uuidIndex = xmlContent.indexOf("uuid");
    if (-1 == uuidIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }
    int quoteStartIndex = xmlContent.indexOf('"', uuidIndex);
    if (-1 == quoteStartIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }
    int quoteEndIndex = xmlContent.indexOf('"', quoteStartIndex + 1);
    if (-1 == quoteEndIndex)
    {
        qWarning() << "Cannot read UUID from file" << fileName << "to set new UUID";
        file.close();
        return;
    }

    file.close();

    QString newXmlContent = xmlContent.left(quoteStartIndex + 1);
    newXmlContent.append(UBStringUtils::toCanonicalUuid(pUuid));
    newXmlContent.append(xmlContent.right(xmlContent.length() - quoteEndIndex));

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QTextStream textWriteStream(&file);
        textWriteStream << newXmlContent;
        file.close();
    }
    else
    {
        qWarning() << "Cannot open file" << fileName  << "to write UUID";
    }
}

bool UBSvgSubsetAdaptor::addElementToBeStored(QString domName, IDataStorage *dataStorageClass)
{
    if(domName.isEmpty() || additionalElementToStore.contains(domName)){
        qWarning() << "Error adding the element that should persist";
        return false;
    }

    additionalElementToStore.insert(domName,dataStorageClass);
    return true;
}

QString UBSvgSubsetAdaptor::uniboardDocumentNamespaceUriFromVersion(int mFileVersion)
{
    return mFileVersion >= 40200 ? UBSettings::uniboardDocumentNamespaceUri : sFormerUniboardDocumentNamespaceUri;
}


UBGraphicsScene* UBSvgSubsetAdaptor::loadScene(UBDocumentProxy* proxy, const int pageIndex)
{
    QString fileName = proxy->persistencePath() + UBFileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);
    qDebug() << "loading scene from" << fileName;
    QFile file(fileName);

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return 0;
        }

        UBGraphicsScene* scene = loadScene(proxy, file.readAll());

        file.close();

        return scene;
    }

    return 0;
}


QUuid UBSvgSubsetAdaptor::sceneUuid(UBDocumentProxy* proxy, const int pageIndex)
{
    QString fileName = proxy->persistencePath() + UBFileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);

    QFile file(fileName);

    QUuid uuid;

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return 0;
        }

        QXmlStreamReader xml(file.readAll());

        bool foundSvg = false;

        while (!xml.atEnd() && !foundSvg)
        {
            xml.readNext();

            if (xml.isStartElement())
            {
                if (xml.name() == "svg")
                {
                    QStringRef svgSceneUuid = xml.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "uuid");
                    if (svgSceneUuid.isNull())
                        svgSceneUuid = xml.attributes().value("http://www.mnemis.com/uniboard", "uuid");

                    if (!svgSceneUuid.isNull())
                        uuid = QUuid(svgSceneUuid.toString());

                    foundSvg = true;
                }
            }
        }

        file.close();
    }

    return uuid;
}


UBGraphicsScene* UBSvgSubsetAdaptor::loadScene(UBDocumentProxy* proxy, const QByteArray& pArray)
{
    UBSvgSubsetReader reader(proxy, UBTextTools::cleanHtmlCData(QString(pArray)).toAscii());
    return reader.loadScene();
}


QDomDocument UBSvgSubsetAdaptor::readTeacherGuideNode(int sceneIndex)
{
    QString fileName = UBApplication::boardController->selectedDocument()->persistencePath() + UBFileSystemUtils::digitFileFormat("/page%1.svg", sceneIndex);
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);
    QByteArray fileByteArray=file.readAll();
    QDomDocument domDocument;
    domDocument.setContent(fileByteArray);
    file.close();

    QDomDocument result("teacherGuide");
    QDomNodeList list = domDocument.childNodes().at(1).childNodes();
    for(int i = 0 ; i < list.size(); i++){
        if(list.at(i).nodeName() == "teacherGuide" || list.at(i).nodeName() == "teacherBar"){
            result.appendChild(list.at(i).cloneNode());
        }
    }
    return result;
}


UBSvgSubsetAdaptor::UBSvgSubsetReader::UBSvgSubsetReader(UBDocumentProxy* pProxy, const QByteArray& pXmlData)
        : mXmlReader(pXmlData)
        , mProxy(pProxy)
        , mDocumentPath(pProxy->persistencePath())
        , mGroupHasInfo(false)
{
    // NOOP
}


UBGraphicsScene* UBSvgSubsetAdaptor::UBSvgSubsetReader::loadScene()
{
    mIsOldVersionFileWithText = false;
    mScene = 0;
    UBGraphicsWidgetItem *currentWidget = 0;

    mFileVersion = 40100; // default to 4.1.0

    UBGraphicsStrokesGroup* strokesGroup = 0;
    UBGraphicsStroke* currentStroke = 0;

    //
    QLinearGradient currentGradient;
    bool isGradient = false;

    while (!mXmlReader.atEnd())
    {
        mXmlReader.readNext();
        if (mXmlReader.isStartElement())
        {
            qreal zFromSvg = getZValueFromSvg();
            QUuid uuidFromSvg = getUuidFromSvg();


            if (mXmlReader.name() == "svg")
            {
                if (!mScene)
                {
                    mScene = new UBGraphicsScene(mProxy, false);
                }

                // introduced in UB 4.2

                QStringRef svgUbVersion = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "version");

                if (!svgUbVersion.isNull())
                {
                    QString ubVersion = svgUbVersion.toString();

                    //may look like : 4 or 4.1 or 4.2 or 4.2.1, etc

                    QStringList parts = ubVersion.split(".");

                    if (parts.length() > 0)
                    {
                        mFileVersion = parts.at(0).toInt() * 10000;
                    }

                    if (parts.length() > 1)
                    {
                        mFileVersion += parts.at(1).toInt() * 100;
                    }

                    if (parts.length() > 2)
                    {
                        mFileVersion += parts.at(2).toInt();
                    }
                }

                mNamespaceUri = uniboardDocumentNamespaceUriFromVersion(mFileVersion);

                QStringRef svgSceneUuid = mXmlReader.attributes().value(mNamespaceUri, "uuid");

                if (!svgSceneUuid.isNull())
                {
                    mScene->setUuid(QUuid(svgSceneUuid.toString()));
                }

                // introduced in UB 4.0

                QStringRef svgViewBox = mXmlReader.attributes().value("viewBox");


                if (!svgViewBox.isNull())
                {
                    QStringList ts = svgViewBox.toString().split(QLatin1Char(' '), QString::SkipEmptyParts);

                    QRectF sceneRect;
                    if (ts.size() >= 4)
                    {
                        sceneRect.setX(ts.at(0).toFloat());
                        sceneRect.setY(ts.at(1).toFloat());
                        sceneRect.setWidth(ts.at(2).toFloat());
                        sceneRect.setHeight(ts.at(3).toFloat());

                        mScene->setSceneRect(sceneRect);
                    }
                    else
                    {
                        qWarning() << "cannot make sense of 'viewBox' value " << svgViewBox.toString();
                    }
                }

                QStringRef pageDpi = mXmlReader.attributes().value("pageDpi");

                if (!pageDpi.isNull())
                    UBSettings::settings()->pageDpi->set(pageDpi.toString());
                else
                    UBSettings::settings()->pageDpi->set(UBApplication::desktop()->physicalDpiX());

                bool darkBackground = false;
                bool crossedBackground = false;

                QStringRef ubDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "dark-background");

                if (!ubDarkBackground.isNull())
                    darkBackground = (ubDarkBackground.toString() == xmlTrue);

                QStringRef ubCrossedBackground = mXmlReader.attributes().value(mNamespaceUri, "crossed-background");

                if (!ubDarkBackground.isNull())
                    crossedBackground = (ubCrossedBackground.toString() == xmlTrue);

                mScene->setBackground(darkBackground, crossedBackground);

                QStringRef pageNominalSize = mXmlReader.attributes().value(mNamespaceUri, "nominal-size");
                if (!pageNominalSize.isNull())
                {
                    QStringList ts = pageNominalSize.toString().split(QLatin1Char('x'), QString::SkipEmptyParts);

                    QSize sceneSize;
                    if (ts.size() >= 2)
                    {
                        sceneSize.setWidth(ts.at(0).toInt());
                        sceneSize.setHeight(ts.at(1).toInt());

                        mScene->setNominalSize(sceneSize);
                    }
                    else
                    {
                        qWarning() << "cannot make sense of 'nominal-size' value " << pageNominalSize.toString();
                    }

                }
            }
            else if (mXmlReader.name() == "g")
            {
                strokesGroup = new UBGraphicsStrokesGroup();
                graphicsItemFromSvg(strokesGroup);

                QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");

                if (!ubZValue.isNull())
                {
                    mGroupZIndex = ubZValue.toString().toFloat();
                    mGroupHasInfo = true;
                }

                QStringRef ubFillOnDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

                if (!ubFillOnDarkBackground.isNull())
                {
                    mGroupDarkBackgroundColor.setNamedColor(ubFillOnDarkBackground.toString());
                }

                QStringRef ubFillOnLightBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

                if (!ubFillOnLightBackground.isNull())
                {
                    mGroupLightBackgroundColor.setNamedColor(ubFillOnLightBackground.toString());
                }
            }
            else if (mXmlReader.name() == "polygon" || mXmlReader.name() == "line")
            {
                UBGraphicsPolygonItem* polygonItem = 0;

                QString parentId = mXmlReader.attributes().value(mNamespaceUri, "parent").toString();

                if (mXmlReader.name() == "polygon")
                {
                    polygonItem = polygonItemFromPolygonSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);
                }
                else if (mXmlReader.name() == "line")
                {
                    polygonItem = polygonItemFromLineSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);
                }
                if(parentId.isEmpty() && strokesGroup)
                    parentId = strokesGroup->uuid().toString();

                if(parentId.isEmpty())
                    parentId = QUuid::createUuid().toString();

                if (polygonItem)
                {
                    polygonItem->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Graphic));

                    UBGraphicsStrokesGroup* group;
                    if(!mStrokesList.contains(parentId)){
                        group = new UBGraphicsStrokesGroup();

                        mStrokesList.insert(parentId,group);
                        currentStroke = new UBGraphicsStroke();
                        group->setTransform(polygonItem->transform());
                        UBGraphicsItem::assignZValue(group, polygonItem->zValue());

                        bool nullondark = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background").isNull();
                        bool nullonlight = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background").isNull();
                        if (nullondark || nullonlight) {
                            QColor curColor(mXmlReader.attributes().value("fill").toString());
                            double opacity = mXmlReader.attributes().value("fill-opacity").toString().toDouble();
                            const char tool = (opacity == 1.0) ? 'p' : 'm';

                            QColor oppozColor(UBApplication::boardController->inferOpposite(curColor, tool));

                            mGroupDarkBackgroundColor = mScene->isDarkBackground() ? curColor : oppozColor;
                            mGroupLightBackgroundColor = mScene->isDarkBackground() ? oppozColor : curColor;

                            polygonItem->setColorOnDarkBackground(mGroupDarkBackgroundColor);
                            polygonItem->setColorOnLightBackground(mGroupLightBackgroundColor);
                        }
                    }
                    else
                        group = mStrokesList.value(parentId);

                    if(polygonItem->transform().isIdentity())
                        polygonItem->setTransform(group->transform());

                    group->addToGroup(polygonItem);
                    polygonItem->setStrokesGroup(group);
                    polygonItem->setStroke(currentStroke);

                    polygonItem->show();
                    group->addToGroup(polygonItem);
                }
            }
            else if (mXmlReader.name() == "polyline")
            {
                QStringRef s = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "shapePath"); // EV-7 - ALTI/AOU - 20140102
                if (!s.isNull())
                {
                    Qt::GlobalColor color = mScene->isDarkBackground() ? Qt::white : Qt::black;
                    UBAbstractGraphicsItem *pathItem = 0;

                    if(s.toString().toInt() == UBEditableGraphicsRegularShapeItem::Type){
                        pathItem = shapeRegularFromSvg(color);
                    }else{
                        pathItem = shapePathFromSvg(color, s.toString().toInt());
                    }

                    if (pathItem)
                    {
                        if (isGradient)
                        {
                            currentGradient.setStart(pathItem->boundingRect().topLeft());
                            currentGradient.setFinalStop(pathItem->boundingRect().topRight());
                            pathItem->setBrush(currentGradient);
                            isGradient = false;
                        }
                        mScene->addItem(pathItem);

                        if (zFromSvg != UBZLayerController::errorNum())
                            UBGraphicsItem::assignZValue(pathItem, zFromSvg);

                        if (!uuidFromSvg.isNull())
                            pathItem->setUuid(uuidFromSvg);
                    }
                }
                else
                {
                    QList<UBGraphicsPolygonItem*> polygonItems = polygonItemsFromPolylineSvg(mScene->isDarkBackground() ? Qt::white : Qt::black);

                    QString parentId = QUuid::createUuid().toString();

                    foreach(UBGraphicsPolygonItem* polygonItem, polygonItems)
                    {
                        polygonItem->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Graphic));

                        UBGraphicsStrokesGroup* group;
                        if(!mStrokesList.contains(parentId)){
                            group = new UBGraphicsStrokesGroup();
                            mStrokesList.insert(parentId,group);
                            currentStroke = new UBGraphicsStroke();
                            group->setTransform(polygonItem->transform());
                            UBGraphicsItem::assignZValue(group, polygonItem->zValue());
                        }
                        else
                            group = mStrokesList.value(parentId);

                        if(polygonItem->transform().isIdentity())
                            polygonItem->setTransform(group->transform());

                        group->addToGroup(polygonItem);
                        polygonItem->setStrokesGroup(group);
                        polygonItem->setStroke(currentStroke);

                        polygonItem->show();
                        group->addToGroup(polygonItem);
                    }
                }
            }
            else if (mXmlReader.name() == "image")
            {
                QStringRef imageHref = mXmlReader.attributes().value(nsXLink, "href");

                if (!imageHref.isNull())
                {
                    QString href = imageHref.toString();

                    QStringRef ubBackground = mXmlReader.attributes().value(mNamespaceUri, "background");
                    bool isBackground = (!ubBackground.isNull() && ubBackground.toString() == xmlTrue);

                    // Issue 1684 - CFA - 20140115
                    // Issue 1684 - ALTI/AOU - 20131210
                    UBFeatureBackgroundDisposition disposition = Center;

                    QStringRef sDisposition = mXmlReader.attributes().value(mNamespaceUri, "disposition");
					if (isBackground)
                    {
                        if (sDisposition.isNull())// centré ou ajusté selon la taille de l'image
                        {
                            int width = mXmlReader.attributes().value("width").toString().toInt();
                            int height = mXmlReader.attributes().value("height").toString().toInt();
                            if (width > mScene->nominalSize().width() || height > mScene->nominalSize().height())
                                disposition = Adjust;
                        }
                        else
                            disposition = static_cast<UBFeatureBackgroundDisposition>(sDisposition.toString().toInt());
                    }
                    // Fin Issue 1684 - ALTI/AOU - 20131210
                    // Fin Issue 1684 - CFA - 20140115

                    if (href.contains("png"))
                    {                        
                        UBGraphicsPixmapItem* pixmapItem = pixmapItemFromSvg();


                        if (pixmapItem)
                        {
                            pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                            pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                            mScene->addItem(pixmapItem);

                            if (zFromSvg != UBZLayerController::errorNum())
                                UBGraphicsItem::assignZValue(pixmapItem, zFromSvg);

                            if (!uuidFromSvg.isNull())
                                pixmapItem->setUuid(uuidFromSvg);

                            if (isBackground)
                            {
                                mScene->setAsBackgroundObject(pixmapItem, true, false, disposition); // Issue 1684 - ALTI/AOU - 20131210
                            }

                            pixmapItem->show();
                        }
                    }
                    else if (href.contains("svg"))
                    {
                        UBGraphicsSvgItem* svgItem = svgItemFromSvg();

                        if (svgItem)
                        {
                            svgItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                            svgItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                            mScene->addItem(svgItem);

                            if (zFromSvg != UBZLayerController::errorNum())
                                UBGraphicsItem::assignZValue(svgItem, zFromSvg);

                            if (isBackground)
                                mScene->setAsBackgroundObject(svgItem);

                            svgItem->show();
                        }
                    }
                    else
                    {
                        qWarning() << "don't know what to do with href value " << href;
                    }
                }
            }
            else if (mXmlReader.name() == "audio")
            {
                UBGraphicsMediaItem* audioItem = audioItemFromSvg();

                if (audioItem)
                {
                    audioItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    audioItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(audioItem);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(audioItem, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        audioItem->setUuid(uuidFromSvg);

                    audioItem->show();

                    //force start to load the video and display the first frame
                    audioItem->mediaObject()->play();
                    audioItem->mediaObject()->pause();
                }
            }
            else if (mXmlReader.name() == "video")
            {
                UBGraphicsMediaItem* videoItem = videoItemFromSvg();

                if (videoItem)
                {
                    videoItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    videoItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(videoItem);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(videoItem, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        videoItem->setUuid(uuidFromSvg);

                    videoItem->show();

                    //force start to load the video and display the first frame
                    videoItem->mediaObject()->play();
                    videoItem->mediaObject()->pause();
                }
            }
            else if (mXmlReader.name() == "text")//This is for backward compatibility with proto text field prior to version 4.3
            {
                UBGraphicsTextItem* textItem = textItemFromSvg();

                if (textItem)
                {
                    textItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                    mScene->addItem(textItem);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(textItem, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        textItem->setUuid(uuidFromSvg);

                    textItem->show();
                }
            }
            else if (mXmlReader.name() == "curtain")
            {
                UBGraphicsCurtainItem* mask = curtainItemFromSvg();

                if (mask)
                {
                    mScene->addItem(mask);
                    mScene->registerTool(mask);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(mask, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        mask->setUuid(uuidFromSvg);
                }
            }
            else if (mXmlReader.name() == "ruler")
            {

                QString ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value").toString();
                UBGraphicsRuler *ruler = rulerFromSvg();

                ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value").toString();
                if (ruler)
                {
                    mScene->addItem(ruler);
                    mScene->registerTool(ruler);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(ruler, zFromSvg);
                }

            }
            else if (mXmlReader.name() == "compass")
            {
                UBGraphicsCompass *compass = compassFromSvg();

                if (compass)
                {
                    mScene->addItem(compass);
                    mScene->registerTool(compass);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(compass, zFromSvg);
                }
            }
            else if (mXmlReader.name() == "protractor")
            {
                UBGraphicsProtractor *protractor = protractorFromSvg();

                if (protractor)
                {
                    mScene->addItem(protractor);
                    mScene->registerTool(protractor);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(protractor, zFromSvg);
                }
            }
            else if (mXmlReader.name() == "triangle")
            {
                UBGraphicsTriangle *triangle = triangleFromSvg();

                if (triangle)
                {
                    mScene->addItem(triangle);
                    mScene->registerTool(triangle);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(triangle, zFromSvg);
                }
            }
            else if (mXmlReader.name() == "cache")
            {
                UBGraphicsCache* cache = cacheFromSvg();
                if(cache)
                {
                    mScene->addItem(cache);
                    mScene->registerTool(cache);
                    UBApplication::boardController->notifyCache(true);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(cache, zFromSvg);
                }
            }
            else if (mXmlReader.name() == "linearGradient")
            {
                currentGradient = linearGradiantFromSvg();
                isGradient = true;
            }
            else if (mXmlReader.name() == "ellipse") // EV-7 - ALTI/AOU - 20131231
            {
                QStringRef isShapeRect = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "shapeEllipse");

                if(!isShapeRect.isNull() && isShapeRect.toString().toLower() == "true"){
                    QStringRef isCircle = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "isCircle");

                    UBAbstractGraphicsItem *item = 0;

                    Qt::GlobalColor color = mScene->isDarkBackground() ? Qt::white : Qt::black;

                    if(!isCircle.isNull() && isCircle.toString().toLower() == "true"){
                        item = shapeCircleFromSvg(color);
                    }else{
                        item = shapeEllipseFromSvg(color);
                    }

                    if (isGradient)
                    {
                        currentGradient.setStart(item->boundingRect().topLeft());
                        currentGradient.setFinalStop(item->boundingRect().topRight());
                        item->setBrush(currentGradient);
                        isGradient = false;
                    }

                    mScene->addItem(item);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(item, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        item->setUuid(uuidFromSvg);
                }
            }
            else if (mXmlReader.name() == "rect") // EV-7 - ALTI/CFA - 20131231
            {
                QStringRef isShapeRect = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "shapeRect");

                if(!isShapeRect.isNull() && isShapeRect.toString().toLower() == "true"){
                    QStringRef isSquare = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "isSquare");

                    UBAbstractGraphicsItem *item = 0;

                    Qt::GlobalColor color = mScene->isDarkBackground() ? Qt::white : Qt::black;

                    if(!isSquare.isNull() && isSquare.toString().toLower() == "true"){
                        item = shapeSquareFromSvg(color);
                    }else{
                        item = shapeRectFromSvg(color);
                    }

                    if (isGradient)
                    {
                        currentGradient.setStart(item->boundingRect().topLeft());
                        currentGradient.setFinalStop(item->boundingRect().topRight());
                        item->setBrush(currentGradient);
                        isGradient = false;
                    }

                    mScene->addItem(item);

                    if (zFromSvg != UBZLayerController::errorNum())
                        UBGraphicsItem::assignZValue(item, zFromSvg);

                    if (!uuidFromSvg.isNull())
                        item->setUuid(uuidFromSvg);
                }
            }
            else if (mXmlReader.name() == "foreignObject")
            {
                QString href = mXmlReader.attributes().value(nsXLink, "href").toString();
                QString src = mXmlReader.attributes().value(mNamespaceUri, "src").toString();
                QString type = mXmlReader.attributes().value(mNamespaceUri, "type").toString();
                bool isBackground = mXmlReader.attributes().value(mNamespaceUri, "background").toString() == xmlTrue;

                qreal foreignObjectWidth = mXmlReader.attributes().value("width").toString().toFloat();
                qreal foreignObjectHeight = mXmlReader.attributes().value("height").toString().toFloat();

                if (href.contains(".pdf"))
                {
                    UBGraphicsPDFItem* pdfItem = pdfItemFromPDF();
                    if (pdfItem)
                    {
                        QDesktopWidget* desktop = UBApplication::desktop();
                        qreal currentDpi = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
                        qreal pdfScale = UBSettings::settings()->pageDpi->get().toReal()/currentDpi;
                        pdfItem->setScale(pdfScale);
                        pdfItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                        pdfItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        mScene->addItem(pdfItem);

                        if (zFromSvg != UBZLayerController::errorNum())
                            UBGraphicsItem::assignZValue(pdfItem, zFromSvg);

                        if (isBackground)
                            mScene->setAsBackgroundObject(pdfItem);

                        pdfItem->show();

                        currentWidget = 0;
                    }
                }
                else if (src.contains(".wdgt"))
                {
                    UBGraphicsAppleWidgetItem* appleWidgetItem = graphicsAppleWidgetFromSvg();
                    if (appleWidgetItem)
                    {
 //                       appleWidgetItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                        appleWidgetItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        appleWidgetItem->resize(foreignObjectWidth, foreignObjectHeight);

                        mScene->addItem(appleWidgetItem);

                        if (zFromSvg != UBZLayerController::errorNum())
                            UBGraphicsItem::assignZValue(appleWidgetItem, zFromSvg);

                        if (!uuidFromSvg.isNull())
                            appleWidgetItem->setUuid(uuidFromSvg);

                        appleWidgetItem->show();

                        currentWidget = appleWidgetItem;
                    }
                }
                else if (src.contains(".wgt"))
                {
                    UBGraphicsW3CWidgetItem* w3cWidgetItem = graphicsW3CWidgetFromSvg();

                    if (w3cWidgetItem)
                    {
                        w3cWidgetItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        w3cWidgetItem->resize(foreignObjectWidth, foreignObjectHeight);

                        mScene->addItem(w3cWidgetItem);

                        if (zFromSvg != UBZLayerController::errorNum())
                            UBGraphicsItem::assignZValue(w3cWidgetItem, zFromSvg);

                        if (!uuidFromSvg.isNull())
                            w3cWidgetItem->setUuid(uuidFromSvg);

                        w3cWidgetItem->show();

                        currentWidget = w3cWidgetItem;
                    }
                }
                else if (type == "text")
                {
                    UBGraphicsTextItem* textItem = textItemFromSvg();

                    UBGraphicsTextItemDelegate *textDelegate = 0;

                    if (textItem)
                        textDelegate = dynamic_cast<UBGraphicsTextItemDelegate*>(textItem->Delegate());

                    if (textDelegate)
                    {
                        QDesktopWidget* desktop = UBApplication::desktop();
                        qreal currentDpi = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
                        qreal textSizeMultiplier = UBSettings::settings()->pageDpi->get().toReal()/currentDpi;
                        textDelegate->scaleTextSize(textSizeMultiplier);
                    }

                    if (textItem)
                    {
                        textItem->setFlag(QGraphicsItem::ItemIsMovable, true);
                        textItem->setFlag(QGraphicsItem::ItemIsSelectable, true);

                        mScene->addItem(textItem);

                        if (zFromSvg != UBZLayerController::errorNum())
                            UBGraphicsItem::assignZValue(textItem, zFromSvg);

                        if (!uuidFromSvg.isNull())
                            textItem->setUuid(uuidFromSvg);

                        textItem->show();
                    }
                }
                else
                {
                    qWarning() << "Ignoring unknown foreignObject:" << href;
                }
            }
            else if (currentWidget && (mXmlReader.name() == "preference"))
            {
                QString key = mXmlReader.attributes().value("key").toString();
                QString value = mXmlReader.attributes().value("value").toString();

                currentWidget->setPreference(key, value);
            }
            else if (currentWidget && (mXmlReader.name() == "datastoreEntry"))
            {
                QString key = mXmlReader.attributes().value("key").toString();
                QString value = mXmlReader.attributes().value("value").toString();

                currentWidget->setDatastoreEntry(key, value);
            } else if (mXmlReader.name() == tGroups) {
                //considering groups section at the end of the document
                readGroupRoot();
            }
            else
            {
                // NOOP
            }
        }
        else if (mXmlReader.isEndElement())
        {
            if (mXmlReader.name() == "g")
            {
                mGroupHasInfo = false;
                mGroupDarkBackgroundColor = Qt::cyan;
                mGroupLightBackgroundColor = Qt::cyan;
            }
        }
    }

    if (mXmlReader.hasError())
    {
        qWarning() << "error parsing Sankore file " << mXmlReader.errorString();
    }

    QHashIterator<QString, UBGraphicsStrokesGroup*> iterator(mStrokesList);
    while (iterator.hasNext()) {
        iterator.next();
        mScene->addItem(iterator.value());
    }

    if (mScene)
        mScene->setModified(false);

    mScene->enableUndoRedoStack();
    return mScene;
}


UBGraphicsGroupContainerItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::readGroup(UBGraphicsItemAction* action, QString uuid)
{
    bool isAnActionForStroke = false;
    UBGraphicsGroupContainerItem *group = new UBGraphicsGroupContainerItem();
    QList<QGraphicsItem *> groupContainer;

    mXmlReader.readNext();
    while (!mXmlReader.atEnd())
    {
        if (mXmlReader.isEndElement()) {
            mXmlReader.readNext();
            break;
        }
        else if (mXmlReader.isStartElement()){
            if (mXmlReader.name() == tGroup)
            {
                UBGraphicsGroupContainerItem *curGroup = readGroup();

                if (curGroup)
                    groupContainer.append(curGroup);
                else
                    qDebug() << "this is an error if readGroup(UBGraphicsItemAction* action, QString uuid)";
            }
            else if (mXmlReader.name() == tElement){
                QString id = mXmlReader.attributes().value(aId).toString();

                if(id == uuid)
                    isAnActionForStroke = true;

                QGraphicsItem *curItem = readElementFromGroup();

                if(curItem  && id.count("{") < 2)
                    groupContainer.append(curItem);
                else
                    qDebug() << "this is an error if readGroup(UBGraphicsItemAction* action, QString uuid)";

            }
            else
                mXmlReader.skipCurrentElement();
        }
        else
            mXmlReader.readNext();
    }

    foreach(QGraphicsItem* item, groupContainer)
        group->addToGroup(item,false);

    if(action && !isAnActionForStroke){
        UBGraphicsGroupContainerItemDelegate* delegate = dynamic_cast<UBGraphicsGroupContainerItemDelegate*>(group->Delegate());
        delegate->setAction(action);
    }


    if (group->childItems().count())
    {
        mScene->addItem(group);

        if (1 == group->childItems().count())
        {
            group->destroy(false);
        }
    }
    return group;
}

void UBSvgSubsetAdaptor::UBSvgSubsetReader::readGroupRoot()
{
    mXmlReader.readNext();
    while (!mXmlReader.atEnd()) {
        if (mXmlReader.isEndElement()) {
            mXmlReader.readNext();
            break;
        }
        else if (mXmlReader.isStartElement()){
            if (mXmlReader.name() == tGroup){
                UBGraphicsItemAction* action = readAction();
                QString ubLocked = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "locked").toString();
                UBGraphicsGroupContainerItem *curGroup = readGroup(action, mXmlReader.attributes().value("id").toString());
                if (!ubLocked.isEmpty())
                {
                    bool isLocked = ubLocked.contains(xmlTrue);
                    curGroup->Delegate()->setLocked(isLocked);
                }
                if (curGroup)
                    mScene->addGroup(curGroup);

            }
            else {
                mXmlReader.skipCurrentElement();
            }
        }
        else {
            mXmlReader.readNext();
        }
    }
}

QGraphicsItem *UBSvgSubsetAdaptor::UBSvgSubsetReader::readElementFromGroup()
{
    QGraphicsItem *result = 0;
    QString id = mXmlReader.attributes().value(aId).toString();
    QString uuid = id.right(QUuid().toString().size());
    result = mScene->itemForUuid(QUuid(uuid));

    if(!result){
        result = mStrokesList.take(uuid.replace("}","").replace("{",""));
    }

    //Q_ASSERT(result);

    mXmlReader.skipCurrentElement();
    mXmlReader.readNext();

    return result;
}

void UBSvgSubsetAdaptor::persistScene(UBDocumentProxy* proxy, UBGraphicsScene* pScene, const int pageIndex)
{
    UBSvgSubsetWriter writer(proxy, pScene, pageIndex);
    writer.persistScene(pageIndex);
}


UBSvgSubsetAdaptor::UBSvgSubsetWriter::UBSvgSubsetWriter(UBDocumentProxy* proxy, UBGraphicsScene* pScene, const int pageIndex)
        : mScene(pScene)
        , mDocumentPath(proxy->persistencePath())
        , mPageIndex(pageIndex)
        , mpDocument(proxy) // Issue 1683 - ALTI/AOU - 20131212
{
    // NOOP
}


void UBSvgSubsetAdaptor::UBSvgSubsetWriter::writeSvgElement()
{
    mXmlWriter.writeStartElement("svg");

    mXmlWriter.writeAttribute("version", "1.1");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "version", UBSettings::currentFileVersion);
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(mScene->uuid()));

    int margin = UBSettings::settings()->svgViewBoxMargin->get().toInt();
    QRect normalized = mScene->normalizedSceneRect().toRect();
    normalized.translate(margin * -1, margin * -1);
    normalized.setWidth(normalized.width() + (margin * 2));
    normalized.setHeight(normalized.height() + (margin * 2));
    mXmlWriter.writeAttribute("viewBox", QString("%1 %2 %3 %4").arg(normalized.x()).arg(normalized.y()).arg(normalized.width()).arg(normalized.height()));

    QSize pageNominalSize = mScene->nominalSize();
    if (pageNominalSize.isValid())
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "nominal-size", QString("%1x%2").arg(pageNominalSize.width()).arg(pageNominalSize.height()));
    }

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "dark-background", mScene->isDarkBackground() ? xmlTrue : xmlFalse);
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "crossed-background", mScene->isCrossedBackground() ? xmlTrue : xmlFalse);

    QDesktopWidget* desktop = UBApplication::desktop();
    mXmlWriter.writeAttribute("pageDpi", QString("%1").arg((desktop->physicalDpiX() + desktop->physicalDpiY()) / 2));


    mXmlWriter.writeStartElement("rect");
    mXmlWriter.writeAttribute("fill", mScene->isDarkBackground() ? "black" : "white");
    mXmlWriter.writeAttribute("x", QString::number(normalized.x()));
    mXmlWriter.writeAttribute("y", QString::number(normalized.y()));
    mXmlWriter.writeAttribute("width", QString::number(normalized.width()));
    mXmlWriter.writeAttribute("height", QString::number(normalized.height()));

    mXmlWriter.writeEndElement();

}

bool UBSvgSubsetAdaptor::UBSvgSubsetWriter::persistScene(int pageIndex)
{
    //issue 1682 - NNE - add the test on the teacherResources
    if (mScene->isModified() || (UBApplication::boardController->paletteManager()->teacherGuideDockWidget() && UBApplication::boardController->paletteManager()->teacherGuideDockWidget()->teacherGuideWidget()->isModified()) ||
            UBApplication::boardController->paletteManager()->teacherResourcesDockWidget() && UBApplication::boardController->paletteManager()->teacherResourcesDockWidget()->isModified())
    {

        //Creating dom structure to store information
        QDomDocument groupDomDocument;
        QDomElement groupRoot = groupDomDocument.createElement(tGroups);
        groupDomDocument.appendChild(groupRoot);

        QBuffer buffer;
        buffer.open(QBuffer::WriteOnly);
        mXmlWriter.setDevice(&buffer);

        mXmlWriter.setAutoFormatting(true);

        mXmlWriter.writeStartDocument();
        mXmlWriter.writeDefaultNamespace(nsSvg);
        mXmlWriter.writeNamespace(nsXLink, "xlink");
        mXmlWriter.writeNamespace(UBSettings::uniboardDocumentNamespaceUri, "ub");
        mXmlWriter.writeNamespace(nsXHtml, "xhtml");

        writeSvgElement();

        // Get the items from the scene
        QList<QGraphicsItem*> items = mScene->items();

        qSort(items.begin(), items.end(), itemZIndexComp);

        UBGraphicsStroke *openStroke = 0;

        bool groupHoldsInfo = false;

        while (!items.empty())
        {
            QGraphicsItem *item = items.takeFirst();

            // Is the item a strokes group?

            UBGraphicsStrokesGroup* strokesGroupItem = qgraphicsitem_cast<UBGraphicsStrokesGroup*>(item);

            bool isFirstItem = true;
            if(strokesGroupItem && strokesGroupItem->isVisible()){
                // Add the polygons
                foreach(QGraphicsItem* item, strokesGroupItem->childItems()){
                    UBGraphicsPolygonItem* poly = qgraphicsitem_cast<UBGraphicsPolygonItem*>(item);
                    if(NULL != poly){
                        polygonItemToSvgPolygon(poly, true, isFirstItem ? strokesGroupItem->Delegate()->action(): 0);
                        items.removeOne(poly);
                        isFirstItem = false;
                    }
                }
            }

            // Is the item a polygon?
            UBGraphicsPolygonItem *polygonItem = qgraphicsitem_cast<UBGraphicsPolygonItem*> (item);
            if (polygonItem && polygonItem->isVisible())
            {

                UBGraphicsStroke* currentStroke = polygonItem->stroke();

                if (openStroke && (currentStroke != openStroke))
                {
                    mXmlWriter.writeEndElement(); //g
                    openStroke = 0;
                    groupHoldsInfo = false;
                }

                bool firstPolygonInStroke = currentStroke  && !openStroke;

                if (firstPolygonInStroke)
                {
                    mXmlWriter.writeStartElement("g");
                    openStroke = currentStroke;

                    QMatrix matrix = item->sceneMatrix();

                    if (!matrix.isIdentity())
                        mXmlWriter.writeAttribute("transform", toSvgTransform(matrix));

                    UBGraphicsStroke* stroke = dynamic_cast<UBGraphicsStroke* >(currentStroke);

                    if (stroke)
                    {
                        QColor colorOnDarkBackground = polygonItem->colorOnDarkBackground();
                        QColor colorOnLightBackground = polygonItem->colorOnLightBackground();

                        if (colorOnDarkBackground.isValid() && colorOnLightBackground.isValid())
                        {
                            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value"
                                                      , QString("%1").arg(polygonItem->zValue()));

                            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                                      , "fill-on-dark-background", colorOnDarkBackground.name());
                            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                                      , "fill-on-light-background", colorOnLightBackground.name());

                            groupHoldsInfo = true;
                        }
                    }

                    if (stroke && !stroke->hasPressure())
                    {

                        strokeToSvgPolyline(stroke, groupHoldsInfo);

                        //we can dequeue all polygons belonging to that stroke
                        foreach(UBGraphicsPolygonItem* gi, stroke->polygons())
                        {
                            items.removeOne(gi);
                        }
                        continue;
                    }
                }

                if (polygonItem->isNominalLine())
                    polygonItemToSvgLine(polygonItem, groupHoldsInfo);
                else
                    polygonItemToSvgPolygon(polygonItem, groupHoldsInfo);

                continue;
            }

            if (openStroke)
            {
                mXmlWriter.writeEndElement(); //g
                groupHoldsInfo = false;
                openStroke = 0;
            }

            // Is the item a picture?
            UBGraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<UBGraphicsPixmapItem*> (item);
            if (pixmapItem && pixmapItem->isVisible())
            {
                //Issue 1684 - CFA - 20131128
                if (pixmapItem->scene()->isBackgroundObject(item))
                   pixmapItemToLinkedImage(pixmapItem, true);
                else
                   pixmapItemToLinkedImage(pixmapItem);
                continue;
            }

            // Is the item a shape?
            UBGraphicsSvgItem *svgItem = qgraphicsitem_cast<UBGraphicsSvgItem*> (item);
            if (svgItem && svgItem->isVisible())
            {
                if (svgItem->scene()->isBackgroundObject(item))
                    svgItemToLinkedSvg(svgItem, true);
                else
                    svgItemToLinkedSvg(svgItem);

                continue;
            }

            UBGraphicsMediaItem *mediaItem = qgraphicsitem_cast<UBGraphicsMediaItem*> (item);

            if (mediaItem && mediaItem->isVisible())
            {
                if (UBGraphicsMediaItem::mediaType_Video == mediaItem->getMediaType())
                    videoItemToLinkedVideo(mediaItem);
                else
                    audioItemToLinkedAudio(mediaItem);
                continue;
            }

            // Is the item an app?
            UBGraphicsAppleWidgetItem *appleWidgetItem = qgraphicsitem_cast<UBGraphicsAppleWidgetItem*> (item);
            if (appleWidgetItem && appleWidgetItem->isVisible())
            {
                graphicsAppleWidgetToSvg(appleWidgetItem);
                continue;
            }

            // Is the item a W3C?
            UBGraphicsW3CWidgetItem *w3cWidgetItem = qgraphicsitem_cast<UBGraphicsW3CWidgetItem*> (item);
            if (w3cWidgetItem && w3cWidgetItem->isVisible())
            {
                graphicsW3CWidgetToSvg(w3cWidgetItem);
                continue;
            }

            // Is the item a PDF?
            UBGraphicsPDFItem *pdfItem = qgraphicsitem_cast<UBGraphicsPDFItem*> (item);
            if (pdfItem && pdfItem->isVisible())
            {
                pdfItemToLinkedPDF(pdfItem);
                continue;
            }

            // Is the item a text?
            UBGraphicsTextItem *textItem = qgraphicsitem_cast<UBGraphicsTextItem*> (item);
            if (textItem && textItem->isVisible())
            {
                textItemToSvg(textItem);
                continue;
            }

            // Is the item a curtain?
            UBGraphicsCurtainItem *curtainItem = qgraphicsitem_cast<UBGraphicsCurtainItem*> (item);
            if (curtainItem && curtainItem->isVisible())
            {
                curtainItemToSvg(curtainItem);
                continue;
            }

            // Is the item a ruler?
            UBGraphicsRuler *ruler = qgraphicsitem_cast<UBGraphicsRuler*> (item);
            if (ruler && ruler->isVisible())
            {
                rulerToSvg(ruler);
                continue;
            }

            // Is the item a cache?
            UBGraphicsCache* cache = qgraphicsitem_cast<UBGraphicsCache*>(item);
            if(cache && cache->isVisible())
            {
                cacheToSvg(cache);
                continue;
            }

            // Is the item a compass
            UBGraphicsCompass *compass = qgraphicsitem_cast<UBGraphicsCompass*> (item);
            if (compass && compass->isVisible())
            {
                compassToSvg(compass);
                continue;
            }

            // Is the item a protractor?
            UBGraphicsProtractor *protractor = qgraphicsitem_cast<UBGraphicsProtractor*> (item);
            if (protractor && protractor->isVisible())
            {
                protractorToSvg(protractor);
                continue;
            }

            // Is the item a triangle?
            UBGraphicsTriangle *triangle = qgraphicsitem_cast<UBGraphicsTriangle*> (item);
            if (triangle && triangle->isVisible())
            {
                triangleToSvg(triangle);
                continue;
            }

            // Is the item a group?
            UBGraphicsGroupContainerItem *groupItem = qgraphicsitem_cast<UBGraphicsGroupContainerItem*>(item);
            if (groupItem && groupItem->isVisible())
            {
                persistGroupToDom(groupItem, &groupRoot, &groupDomDocument,groupItem->Delegate()->action());
                continue;
            }

            // Is the item a shape Ellipse ?
            UB3HEditableGraphicsEllipseItem* shapeEllipseItem = dynamic_cast<UB3HEditableGraphicsEllipseItem*>(item);// EV-7 - ALTI/AOU - 20131231
            if (shapeEllipseItem && shapeEllipseItem->isVisible())
            {
                shapeEllipseToSvg(shapeEllipseItem);
            }

            // Is the item a shape Rect ?
            UB3HEditableGraphicsRectItem* shapeRectItem = dynamic_cast<UB3HEditableGraphicsRectItem*>(item);// EV-7 - ALTI/AOU - 20131231
            if (shapeRectItem && shapeRectItem->isVisible())
            {
                shapeRectToSvg(shapeRectItem);
            }

            // Is the item a shape Square ?
            UB1HEditableGraphicsSquareItem* shapeSquareItem = dynamic_cast<UB1HEditableGraphicsSquareItem*>(item);// EV-7 - ALTI/AOU - 20131231
            if (shapeSquareItem && shapeSquareItem->isVisible())
            {
                shapeSquareToSvg(shapeSquareItem);
            }

            // Is the item a shape Circle ?
            UB1HEditableGraphicsCircleItem* shapeCircleItem = dynamic_cast<UB1HEditableGraphicsCircleItem*>(item);// EV-7 - ALTI/AOU - 20131231
            if (shapeCircleItem && shapeCircleItem->isVisible())
            {
                shapeCircleToSvg(shapeCircleItem);
            }


            // Is the item a shape Path ? (closed polygon, opened polygon, freehand drawing)
            UBAbstractGraphicsPathItem * shapepathItem = dynamic_cast<UBAbstractGraphicsPathItem *>(item); // EV-7 - ALTI/AOU - 20140102
            if (shapepathItem && shapepathItem->isVisible())
            {
                shapePathToSvg(shapepathItem);
            }

            // Is the item a shape Path ? (closed polygon, opened polygon, freehand drawing)
            UBEditableGraphicsRegularShapeItem * shapeRegularItem = dynamic_cast<UBEditableGraphicsRegularShapeItem *>(item); // EV-7 - ALTI/AOU - 20140102
            if (shapeRegularItem && shapeRegularItem->isVisible())
            {
                shapeRegularToSvg(shapeRegularItem);
            }
        }

        if (openStroke)
        {
            mXmlWriter.writeEndElement();
            groupHoldsInfo = false;
            openStroke = 0;
        }

        QMap<QString,IDataStorage*> elements = getAdditionalElementToStore();
        QVector<tIDataStorage*> dataStorageItems;

        //issue 1682 - NNE - 20140122 : Refractor the persistance for the teacher and
        //the resources guide
        tIDataStorage* data = new tIDataStorage("teacherGuide", eElementType_START);
        data->attributes.insert("version", "2.3.0");
        dataStorageItems << data;

        if(pageIndex != 0){
            if(elements.value("teacherGuide"))
                dataStorageItems += elements.value("teacherGuide")->save(pageIndex);

            if(elements.value("resourcesGuide"))
                dataStorageItems += elements.value("resourcesGuide")->save(pageIndex);
        }

        // Issue 1683 - ALTI/AOU - 20131212
        // On ne peut malheureusement pas utiliser UBTeacherGuidePageZeroWidget.save(),
        // car celle-ci ne pourrait renvoyer que les élements du document chargé dans la Board.
        // Or on veut pouvoir ici persister un autre Document, car par exemple quand on crée un nouveau Document :
        // on le persite alors qu'il n'a pas encore été chargé dans la Board.
        if(pageIndex == 0)
        {
            QVector<tIDataStorage*> result;

            foreach (UBDocumentExternalFile* ef, *(mpDocument->externalFiles())) {
                data = new tIDataStorage();
                data->name = "file";
                data->type = eElementType_UNIQUE;
                data->attributes.insert("path", ef->path());
                data->attributes.insert("title", ef->title());
                result << data;
            }

            dataStorageItems += result;
        }
        // Fin Issue 1683 - ALTI/AOU - 20131212

        dataStorageItems << new tIDataStorage("teacherGuide", eElementType_END);
        //issue 1682 - NNE - 20140122 : END

        foreach(tIDataStorage* eachItem, dataStorageItems){
            if(eachItem->type == eElementType_START){
                mXmlWriter.writeStartElement(eachItem->name);
                foreach(QString key,eachItem->attributes.keys())
                    mXmlWriter.writeAttribute(key,eachItem->attributes.value(key));
            }
            else if (eachItem->type == eElementType_END)
                mXmlWriter.writeEndElement();
            else if (eachItem->type == eElementType_UNIQUE){
                mXmlWriter.writeStartElement(eachItem->name);
                foreach(QString key,eachItem->attributes.keys())
                    mXmlWriter.writeAttribute(key,eachItem->attributes.value(key));
                mXmlWriter.writeEndElement();
            }
            else
                qWarning() << "unknown type";
        }


        //writing group data
        if (groupRoot.hasChildNodes()) {
            mXmlWriter.writeStartElement(tGroups);
            QDomElement curElement = groupRoot.firstChildElement();
            while (!curElement.isNull()) {
                //hack
                if (curElement.hasAttribute(aId)) {
                    mXmlWriter.writeStartElement(curElement.tagName());
                    mXmlWriter.writeAttribute(aId, curElement.attribute(aId));
                    if(curElement.hasAttribute("actionType")){
                        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionType",curElement.attribute("actionType"));
                        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionFirstParameter",curElement.attribute("actionFirstParameter"));
                        if(curElement.hasAttribute("actionSecondParameter"))
                            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionSecondParameter",curElement.attribute("actionSecondParameter"));
                    }
                    //hack
                    if(curElement.hasAttribute("locked")){
                        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"locked",curElement.attribute("locked"));
                    }
                    QDomElement curSubElement = curElement.firstChildElement();
                    while (!curSubElement.isNull()) {
                        if (curSubElement.hasAttribute(aId)) {
                            mXmlWriter.writeStartElement(curSubElement.tagName());
                            mXmlWriter.writeAttribute(aId, curSubElement.attribute(aId));
                            mXmlWriter.writeEndElement();
                            curSubElement = curSubElement.nextSiblingElement();
                        }
                    }
                    mXmlWriter.writeEndElement();
                }
                curElement = curElement.nextSiblingElement();
            }
            mXmlWriter.writeEndElement();
        }

        mXmlWriter.writeEndDocument();
        QString fileName = mDocumentPath + UBFileSystemUtils::digitFileFormat("/page%1.svg", mPageIndex);
        QFile file(fileName);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            qCritical() << "cannot open " << fileName << " for writing ...";
            return false;
        }
        file.write(buffer.data());
        file.flush();
        file.close();

    }
    else
    {
        qDebug() << "ignoring unmodified page" << UBApplication::boardController->pageFromSceneIndex(mPageIndex);
    }

    return true;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::persistGroupToDom(QGraphicsItem *groupItem, QDomElement *curParent, QDomDocument *groupDomDocument, UBGraphicsItemAction* action)
{
    QUuid uuid = UBGraphicsScene::getPersonalUuid(groupItem);
    if (!uuid.isNull()) {
        QDomElement curGroupElement = groupDomDocument->createElement(tGroup);
        curGroupElement.setAttribute(aId, uuid);
        //persist delegate properties
        UBGraphicsGroupContainerItem* group = dynamic_cast<UBGraphicsGroupContainerItem*>(groupItem);
        if(group && group->Delegate()){
            if(group->Delegate()->isLocked())
                curGroupElement.setAttribute("locked", xmlTrue);
            else
                curGroupElement.setAttribute("locked", xmlFalse);
        }

        if(action){
            QStringList actionParameter = action->save();
            curGroupElement.setAttribute("actionType",actionParameter.at(0));
            curGroupElement.setAttribute("actionFirstParameter",actionParameter.at(1));
            if(actionParameter.count() > 2)
                curGroupElement.setAttribute("actionSecondParameter",actionParameter.at(2));
        }


        curParent->appendChild(curGroupElement);

        foreach (QGraphicsItem *item, groupItem->childItems()) {
            QUuid tmpUuid = UBGraphicsScene::getPersonalUuid(item);
            if (!tmpUuid.isNull()) {
                if (item->type() == UBGraphicsGroupContainerItem::Type && item->childItems().count()) {
                    persistGroupToDom(item, curParent, groupDomDocument);
                }
                else {
                    QDomElement curSubElement = groupDomDocument->createElement(tElement);

                    curSubElement.setAttribute(aId, tmpUuid);
                    curGroupElement.appendChild(curSubElement);
                }
            }
        }
    }
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::polygonItemToSvgLine(UBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo)
{
    mXmlWriter.writeStartElement("line");

    QLineF line = polygonItem->originalLine();

    mXmlWriter.writeAttribute("x1", QString::number(line.p1().x(), 'f', 2));
    mXmlWriter.writeAttribute("y1", QString::number(line.p1().y(), 'f', 2));

    // SVG renderers (Chrome) do not like line where (x1, y1) == (x2, y2)
    qreal x2 = line.p2().x();
    if (line.p1() == line.p2())
        x2 += 0.01;

    mXmlWriter.writeAttribute("x2", QString::number(x2, 'f', 2));
    mXmlWriter.writeAttribute("y2", QString::number(line.p2().y(), 'f', 2));

    mXmlWriter.writeAttribute("stroke-width", QString::number(polygonItem->originalWidth(), 'f', -1));
    mXmlWriter.writeAttribute("stroke", polygonItem->brush().color().name());

    qreal alpha = polygonItem->brush().color().alphaF();
    if (alpha < 1.0)
        mXmlWriter.writeAttribute("stroke-opacity", QString::number(alpha, 'f', 2));
    mXmlWriter.writeAttribute("stroke-linecap", "round");

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "fill-on-dark-background", polygonItem->colorOnDarkBackground().name());
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "fill-on-light-background", polygonItem->colorOnLightBackground().name());

    if (!groupHoldsInfo)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", QString("%1").arg(polygonItem->zValue()));
    }

    mXmlWriter.writeEndElement();

}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::strokeToSvgPolyline(UBGraphicsStroke* stroke, bool groupHoldsInfo)
{
    QList<UBGraphicsPolygonItem*> pols = stroke->polygons();

    if (pols.length() > 0)
    {
        mXmlWriter.writeStartElement("polyline");
        QVector<QPointF> points;

        foreach(UBGraphicsPolygonItem* polygon, pols)
        {
            points << polygon->originalLine().p1();
        }

        points << pols.last()->originalLine().p2();

        // SVG renderers (Chrome) do not like line withe where x1/y1 == x2/y2
        if (points.size() == 2 && (points.at(0) == points.at(1)))
        {
            points[1] = QPointF(points[1].x() + 0.01, points[1].y());
        }

        QString svgPoints = pointsToSvgPointsAttribute(points);
        mXmlWriter.writeAttribute("points", svgPoints);

        UBGraphicsPolygonItem* firstPolygonItem = pols.at(0);

        mXmlWriter.writeAttribute("fill", "none");
        mXmlWriter.writeAttribute("stroke-width", QString::number(firstPolygonItem->originalWidth(), 'f', 2));
        mXmlWriter.writeAttribute("stroke", firstPolygonItem->brush().color().name());
        mXmlWriter.writeAttribute("stroke-opacity", QString("%1").arg(firstPolygonItem->brush().color().alphaF()));
        mXmlWriter.writeAttribute("stroke-linecap", "round");

        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                  , "fill-on-dark-background", firstPolygonItem->colorOnDarkBackground().name());
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                  , "fill-on-light-background", firstPolygonItem->colorOnLightBackground().name());

        if (!groupHoldsInfo)
        {

            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", QString("%1").arg(firstPolygonItem->zValue()));


        }

        mXmlWriter.writeEndElement();
    }
}


void UBSvgSubsetAdaptor::UBSvgSubsetWriter::strokeToSvgPolygon(UBGraphicsStroke* stroke, bool groupHoldsInfo)
{
    QList<UBGraphicsPolygonItem*> pis = stroke->polygons();

    if (pis.length() > 0)
    {
        QPolygonF united;

        foreach(UBGraphicsPolygonItem* pi, pis)
        {
            united = united.united(pi->polygon());
        }


        UBGraphicsPolygonItem *clone = static_cast<UBGraphicsPolygonItem*>(pis.at(0)->deepCopy());
        clone->setPolygon(united);

        polygonItemToSvgPolygon(clone, groupHoldsInfo);
    }
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::polygonItemToSvgPolygon(UBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo, UBGraphicsItemAction* action)
{

    QPolygonF polygon = polygonItem->polygon();
    int pointsCount = polygon.size();

    if (polygonItem && pointsCount > 0)
    {
        mXmlWriter.writeStartElement("polygon");

        QString points = pointsToSvgPointsAttribute(polygon);
        mXmlWriter.writeAttribute("points", points);
        mXmlWriter.writeAttribute("transform",toSvgTransform(polygonItem->sceneMatrix()));
        mXmlWriter.writeAttribute("fill", polygonItem->brush().color().name());
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                  , "fill-on-dark-background", polygonItem->colorOnDarkBackground().name());
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                  , "fill-on-light-background", polygonItem->colorOnLightBackground().name());

        qreal alpha = polygonItem->brush().color().alphaF();
        mXmlWriter.writeAttribute("fill-opacity", QString::number(alpha, 'f', 2));

        // we trick SVG antialiasing, to avoid seeing light gaps between polygons
        if (alpha < 1.0)
        {
            qreal trickedAlpha = trickAlpha(alpha);
            mXmlWriter.writeAttribute("stroke", polygonItem->brush().color().name());
            mXmlWriter.writeAttribute("stroke-width", "1");
            mXmlWriter.writeAttribute("stroke-opacity", QString::number(trickedAlpha, 'f', 2));
        }

        // svg default fill rule is nonzero, but Qt is evenodd
        //
        //http://www.w3.org/TR/SVG11/painting.html
        //http://doc.trolltech.com/4.5/qgraphicspolygonitem.html#fillRule
        //

        if (polygonItem->fillRule() == Qt::OddEvenFill)
            mXmlWriter.writeAttribute("fill-rule", "evenodd");

        if (!groupHoldsInfo)
        {
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", QString("%1").arg(polygonItem->zValue()));
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                      , "fill-on-dark-background", polygonItem->colorOnDarkBackground().name());
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                                      , "fill-on-light-background", polygonItem->colorOnLightBackground().name());
        }


        if(action)
            writeAction(action);

        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(polygonItem->uuid()));
        if (polygonItem->parentItem()) {
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "parent", UBStringUtils::toCanonicalUuid(UBGraphicsItem::getOwnUuid(polygonItem->parentItem())));
        }

        mXmlWriter.writeEndElement();
    }
}

UBGraphicsPolygonItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::polygonItemFromPolygonSvg(const QColor& pDefaultColor)
{
    UBGraphicsPolygonItem* polygonItem = new UBGraphicsPolygonItem();

    QStringRef svgPoints = mXmlReader.attributes().value("points");

    QPolygonF polygon;

    if (!svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '),
                         QString::SkipEmptyParts);

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), QString::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                polygon << point;
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                polygon << point;
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    polygonItem->setPolygon(polygon);

    QStringRef svgTransform = mXmlReader.attributes().value("transform");

    QMatrix itemMatrix;

    if (!svgTransform.isNull())
    {
        itemMatrix = fromSvgTransform(svgTransform.toString());
        polygonItem->setMatrix(itemMatrix);
    }

    QStringRef svgFill = mXmlReader.attributes().value("fill");

    QColor brushColor = pDefaultColor;

    if (!svgFill.isNull())
    {
        brushColor.setNamedColor(svgFill.toString());

    }

    QStringRef svgFillOpacity = mXmlReader.attributes().value("fill-opacity");
    qreal opacity = 1.0;

    if (!svgFillOpacity.isNull())
    {
        opacity = svgFillOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    polygonItem->setColor(brushColor);

    QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    if (!ubZValue.isNull())
    {
        UBGraphicsItem::assignZValue(polygonItem, ubZValue.toString().toFloat());
    }
    else
    {
        UBGraphicsItem::assignZValue(polygonItem, mGroupZIndex);
    }

    QStringRef ubFillOnDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

    if (!ubFillOnDarkBackground.isNull())
    {
        QColor color;
        color.setNamedColor(ubFillOnDarkBackground.toString());
        if (!color.isValid())
            color = Qt::white;

        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }
    else
    {
        QColor color = mGroupDarkBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }

    QStringRef ubFillOnLightBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!ubFillOnLightBackground.isNull())
    {
        QColor color;
        color.setNamedColor(ubFillOnLightBackground.toString());
        if (!color.isValid())
            color = Qt::black;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }
    else
    {
        QColor color = mGroupLightBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }
    return polygonItem;

}

UBGraphicsPolygonItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::polygonItemFromLineSvg(const QColor& pDefaultColor)
{
    QStringRef svgX1 = mXmlReader.attributes().value("x1");
    QStringRef svgY1 = mXmlReader.attributes().value("y1");
    QStringRef svgX2 = mXmlReader.attributes().value("x2");
    QStringRef svgY2 = mXmlReader.attributes().value("y2");

    QLineF line;

    if (!svgX1.isNull() && !svgY1.isNull() && !svgX2.isNull() && !svgY2.isNull())
    {
        qreal x1 = svgX1.toString().toFloat();
        qreal y1 = svgY1.toString().toFloat();
        qreal x2 = svgX2.toString().toFloat();
        qreal y2 = svgY2.toString().toFloat();

        line.setLine(x1, y1, x2, y2);
    }
    else
    {
        qWarning() << "cannot make sense of 'line' value";
        return 0;
    }

    QStringRef strokeWidth = mXmlReader.attributes().value("stroke-width");

    qreal lineWidth = 1.;

    if (!strokeWidth.isNull())
    {
        lineWidth = strokeWidth.toString().toFloat();
    }

    UBGraphicsPolygonItem* polygonItem = new UBGraphicsPolygonItem(line, lineWidth);

    QStringRef svgStroke = mXmlReader.attributes().value("stroke");

    QColor brushColor = pDefaultColor;

    if (!svgStroke.isNull())
    {
        brushColor.setNamedColor(svgStroke.toString());

    }

    QStringRef svgStrokeOpacity = mXmlReader.attributes().value("stroke-opacity");
    qreal opacity = 1.0;

    if (!svgStrokeOpacity.isNull())
    {
        opacity = svgStrokeOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    polygonItem->setColor(brushColor);

    QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    if (!ubZValue.isNull())
    {
        UBGraphicsItem::assignZValue(polygonItem, ubZValue.toString().toFloat());
    }
    else
    {
        UBGraphicsItem::assignZValue(polygonItem, mGroupZIndex);
    }


    QStringRef ubFillOnDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");

    if (!ubFillOnDarkBackground.isNull())
    {
        QColor color;
        color.setNamedColor(ubFillOnDarkBackground.toString());
        if (!color.isValid())
            color = Qt::white;

        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }
    else
    {
        QColor color = mGroupDarkBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnDarkBackground(color);
    }

    QStringRef ubFillOnLightBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!ubFillOnLightBackground.isNull())
    {
        QColor color;
        color.setNamedColor(ubFillOnLightBackground.toString());
        if (!color.isValid())
            color = Qt::black;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }
    else
    {
        QColor color = mGroupLightBackgroundColor;
        color.setAlphaF(opacity);
        polygonItem->setColorOnLightBackground(color);
    }

    return polygonItem;
}

QList<UBGraphicsPolygonItem*> UBSvgSubsetAdaptor::UBSvgSubsetReader::polygonItemsFromPolylineSvg(const QColor& pDefaultColor)
{
    QStringRef strokeWidth = mXmlReader.attributes().value("stroke-width");

    qreal lineWidth = 1.;

    if (!strokeWidth.isNull())
    {
        lineWidth = strokeWidth.toString().toFloat();
    }

    QColor brushColor = pDefaultColor;

    QStringRef svgStroke = mXmlReader.attributes().value("stroke");
    if (!svgStroke.isNull())
    {
        brushColor.setNamedColor(svgStroke.toString());
    }

    qreal opacity = 1.0;

    QStringRef svgStrokeOpacity = mXmlReader.attributes().value("stroke-opacity");
    if (!svgStrokeOpacity.isNull())
    {
        opacity = svgStrokeOpacity.toString().toFloat();
        brushColor.setAlphaF(opacity);
    }

    QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    qreal zValue = mGroupZIndex;
    if (!ubZValue.isNull())
    {
        zValue = ubZValue.toString().toFloat();
    }

    QColor colorOnDarkBackground = mGroupDarkBackgroundColor;

    QStringRef ubFillOnDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");
    if (!ubFillOnDarkBackground.isNull())
    {
        colorOnDarkBackground.setNamedColor(ubFillOnDarkBackground.toString());
    }

    if (!colorOnDarkBackground.isValid())
        colorOnDarkBackground = Qt::white;

    colorOnDarkBackground.setAlphaF(opacity);

    QColor colorOnLightBackground = mGroupLightBackgroundColor;

    QStringRef ubFillOnLightBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");
    if (!ubFillOnLightBackground.isNull())
    {
        QColor colorOnLightBackground;
        colorOnLightBackground.setNamedColor(ubFillOnLightBackground.toString());
    }

    if (!colorOnLightBackground.isValid())
        colorOnLightBackground = Qt::black;

    colorOnLightBackground.setAlphaF(opacity);

    QStringRef svgPoints = mXmlReader.attributes().value("points");

    QList<UBGraphicsPolygonItem*> polygonItems;

    if (!svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '),
                         QString::SkipEmptyParts);

        QList<QPointF> points;

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), QString::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                points << point;
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                points << point;
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }
        }

        for (int i = 0; i < points.size() - 1; i++)
        {
            UBGraphicsPolygonItem* polygonItem = new UBGraphicsPolygonItem(QLineF(points.at(i), points.at(i + 1)), lineWidth);
            polygonItem->setColor(brushColor);
            UBGraphicsItem::assignZValue(polygonItem, zValue);
            polygonItem->setColorOnDarkBackground(colorOnDarkBackground);
            polygonItem->setColorOnLightBackground(colorOnLightBackground);

            polygonItems <<polygonItem;
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    return polygonItems;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::pixmapItemToLinkedImage(UBGraphicsPixmapItem* pixmapItem, bool isBackground)
{
    mXmlWriter.writeStartElement("image");
    QString fileName;
    if (isBackground // Issue 1684 - CFA - 20131128 : specify isBackground
        && ( ! mScene->document()->metaData(UBSettings::documentDefaultBackgroundImage).toString().isEmpty())) // Issue 1684 - ALTI/AOU - 20131210 : Si il y a une image par défaut définie, on utilise son uuid :
    {
        fileName = UBPersistenceManager::imageDirectory + "/" + mScene->document()->metaData(UBSettings::documentDefaultBackgroundImage).toString();
    }
    else
        fileName = UBPersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";

    QString path = mDocumentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(mDocumentPath + "/" + UBPersistenceManager::imageDirectory);

        pixmapItem->pixmap().toImage().save(path, "PNG");
    }

    mXmlWriter.writeAttribute(nsXLink, "href", fileName);

    writeAction(pixmapItem->Delegate()->action());
    graphicsItemToSvg(pixmapItem);

    mXmlWriter.writeEndElement();
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::writeAction(UBGraphicsItemAction* action)
{
    if(action){
        QStringList actionParameters = action->save();
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionType",actionParameters.at(0));
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionFirstParameter",actionParameters.at(1));
        if(actionParameters.count()>2)
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri,"actionSecondParameter",actionParameters.at(2));
    }
}

UBGraphicsItemAction* UBSvgSubsetAdaptor::UBSvgSubsetReader::readAction()
{
    UBGraphicsItemAction* result = 0;
    QXmlStreamAttributes attributes = mXmlReader.attributes();
    QStringRef actionParameters = attributes.value(UBSettings::uniboardDocumentNamespaceUri, "actionType");
    if(actionParameters.isEmpty())
        return result;
    switch (actionParameters.toString().toInt()) {
    case eLinkToAudio:{
        //NNE - 20141106 : Correction du bug lorsque l'on charge un ubz en cliquant dessus
        qDebug() << mProxy->persistencePath();

        QString audioPath = mProxy->persistencePath() + "/" + attributes.value(UBSettings::uniboardDocumentNamespaceUri, "actionFirstParameter").toString();

        UBGraphicsItemPlayAudioAction *item = new UBGraphicsItemPlayAudioAction(audioPath, false);

        result = item;
        break;
        //NNE - 20141106 : END
    }case eLinkToPage:{
        QString pageParameter = attributes.value(UBSettings::uniboardDocumentNamespaceUri, "actionSecondParameter").toString();
        if(pageParameter.isEmpty())
            result = new UBGraphicsItemMoveToPageAction((eUBGraphicsItemMovePageAction)attributes.value(UBSettings::uniboardDocumentNamespaceUri, "actionFirstParameter").toString().toInt());
        else
            result = new UBGraphicsItemMoveToPageAction((eUBGraphicsItemMovePageAction)attributes.value(UBSettings::uniboardDocumentNamespaceUri, "actionFirstParameter").toString().toInt(),pageParameter.toInt());
        break;
    }
    case eLinkToWebUrl:
        result = new UBGraphicsItemLinkToWebPageAction(mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "actionFirstParameter").toString());
        break;
    default:
        break;
    }

    return result;
}

UBGraphicsPixmapItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::pixmapItemFromSvg()
{

    UBGraphicsPixmapItem* pixmapItem = new UBGraphicsPixmapItem();

    QStringRef imageHref = mXmlReader.attributes().value(nsXLink, "href");

    if (!imageHref.isNull())
    {
        QString href = imageHref.toString();
        QPixmap pix(mDocumentPath + "/" + UBFileSystemUtils::normalizeFilePath(href));
        pixmapItem->setPixmap(pix);
    }
    else
    {
        qWarning() << "cannot make sens of image href value";
        return 0;
    }

    graphicsItemFromSvg(pixmapItem);

    pixmapItem->Delegate()->setAction(readAction());
    return pixmapItem;

}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::svgItemToLinkedSvg(UBGraphicsSvgItem* svgItem, bool isBackground)
{

    mXmlWriter.writeStartElement("image");

    QString fileName;
    if (isBackground
            && ( ! mScene->document()->metaData(UBSettings::documentDefaultBackgroundImage).toString().isEmpty())) // Issue 1684 - ALTI/AOU - 20131210 : Si il y a une image par défaut définie, on utilise son uuid :
    {
        fileName = UBPersistenceManager::imageDirectory + "/" + mScene->document()->metaData(UBSettings::documentDefaultBackgroundImage).toString();
    }
    else
        fileName = UBPersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";

    QString path = mDocumentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(mDocumentPath + "/" + UBPersistenceManager::imageDirectory);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "cannot open file for writing embeded svg content " << path;
            return;
        }

        file.write(svgItem->fileData());
    }

    mXmlWriter.writeAttribute(nsXLink, "href", fileName);

    writeAction(svgItem->Delegate()->action());

    graphicsItemToSvg(svgItem);

    mXmlWriter.writeEndElement();
}

UBGraphicsSvgItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::svgItemFromSvg()
{
    UBGraphicsSvgItem* svgItem = 0;

    QStringRef imageHref = mXmlReader.attributes().value(nsXLink, "href");

    if (!imageHref.isNull())
    {
        QString href = imageHref.toString();

        svgItem = new UBGraphicsSvgItem(mDocumentPath + "/" + UBFileSystemUtils::normalizeFilePath(href));        
    }
    else
    {
        qWarning() << "cannot make sens of image href value";
        return 0;
    }

    graphicsItemFromSvg(svgItem);
    svgItem->Delegate()->setAction(readAction());

    return svgItem;

}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::pdfItemToLinkedPDF(UBGraphicsPDFItem* pdfItem)
{
    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute("requiredExtensions", "http://ns.adobe.com/pdf/1.3/");

    QString fileName = UBPersistenceManager::objectDirectory + "/" + pdfItem->fileUuid().toString() + ".pdf";

    QString path = mDocumentPath + "/" + fileName;

    if (!QFile::exists(path))
    {
        QDir dir;
        dir.mkdir(mDocumentPath + "/" + UBPersistenceManager::objectDirectory);

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
        {
            qWarning() << "cannot open file for writing embeded pdf content " << path;
            return;
        }

        file.write(pdfItem->fileData());
    }

    mXmlWriter.writeAttribute(nsXLink, "href", fileName + "#page=" + QString::number(pdfItem->pageNumber()));

    graphicsItemToSvg(pdfItem);

    mXmlWriter.writeEndElement();
}

UBGraphicsPDFItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::pdfItemFromPDF()
{
    UBGraphicsPDFItem* pdfItem = 0;

    QString href = mXmlReader.attributes().value(nsXLink, "href").toString();
    QStringList parts = href.split("#page=");
    if (parts.count() != 2)
    {
        qWarning() << "invalid pdf href value" << href;
        return 0;
    }

    QString pdfPath = parts[0];
    QUuid uuid(QFileInfo(pdfPath).baseName());
    int pageNumber = parts[1].toInt();

    pdfItem = new UBGraphicsPDFItem(PDFRenderer::rendererForUuid(uuid, mDocumentPath + "/" + UBFileSystemUtils::normalizeFilePath(pdfPath)), pageNumber);

    graphicsItemFromSvg(pdfItem);

    return pdfItem;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::audioItemToLinkedAudio(UBGraphicsMediaItem* audioItem)
{
    mXmlWriter.writeStartElement("audio");

    graphicsItemToSvg(audioItem);

    if (audioItem->mediaObject()->state() == Phonon::PausedState && audioItem->mediaObject()->remainingTime() > 0)
    {
        qint64 pos = audioItem->mediaObject()->currentTime();
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "position", QString("%1").arg(pos));
    }

    QString audioFileHref = audioItem->mediaFileUrl().toString();
    audioFileHref = UBFileSystemUtils::removeLocalFilePrefix(audioFileHref);
    if(audioFileHref.startsWith(mDocumentPath))
        audioFileHref = audioFileHref.replace(mDocumentPath + "/","");

    mXmlWriter.writeAttribute(nsXLink, "href", audioFileHref);
    mXmlWriter.writeEndElement();
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::videoItemToLinkedVideo(UBGraphicsMediaItem* videoItem)
{
    /* w3c sample
     *
     *  <video xlink:href="noonoo.avi" volume=".8" type="video/x-msvideo"
     *               width="320" height="240" x="50" y="50" repeatCount="indefinite"/>
     *
     */

    mXmlWriter.writeStartElement("video");

    graphicsItemToSvg(videoItem);

    if (videoItem->mediaObject()->state() == Phonon::PausedState && videoItem->mediaObject()->remainingTime() > 0)
    {
        qint64 pos = videoItem->mediaObject()->currentTime();
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "position", QString("%1").arg(pos));
    }

    QString videoFileHref = videoItem->mediaFileUrl().toString();

    videoFileHref = UBFileSystemUtils::removeLocalFilePrefix(videoFileHref);
    if(videoFileHref.startsWith(mDocumentPath))
        videoFileHref = videoFileHref.replace(mDocumentPath + "/","");

    mXmlWriter.writeAttribute(nsXLink, "href", videoFileHref);
    mXmlWriter.writeEndElement();
}

UBGraphicsMediaItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::audioItemFromSvg()
{

    QStringRef audioHref = mXmlReader.attributes().value(nsXLink, "href");

    if (audioHref.isNull())
    {
        qWarning() << "cannot make sens of video href value";
        return 0;
    }

    QString href = mDocumentPath + "/" + audioHref.toString();

    //Claudio this is necessary to fix the absolute path added on Sankore 3.1 1.00.00
    //The absoult path doesn't work when you want to share Sankore documents.
    if(!href.startsWith("audios/")){
        int indexOfAudioDirectory = href.lastIndexOf("audios");
        href = mDocumentPath + "/" + href.right(href.length() - indexOfAudioDirectory);
    }

    UBGraphicsMediaItem* audioItem = new UBGraphicsMediaItem(QUrl::fromLocalFile(href));
    if(audioItem){
        audioItem->connect(UBApplication::boardController, SIGNAL(activeSceneChanged()), audioItem, SLOT(activeSceneChanged()));
    }

    graphicsItemFromSvg(audioItem);
    QStringRef ubPos = mXmlReader.attributes().value(mNamespaceUri, "position");

    qint64 p = 0;
    if (!ubPos.isNull())
    {
        p = ubPos.toString().toLongLong();
    }

    audioItem->setInitialPos(p);
    return audioItem;
}

UBGraphicsMediaItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::videoItemFromSvg()
{

    QStringRef videoHref = mXmlReader.attributes().value(nsXLink, "href");

    if (videoHref.isNull())
    {
        qWarning() << "cannot make sens of video href value";
        return 0;
    }

    QString href = mDocumentPath + "/" + videoHref.toString();

    //Claudio this is necessary to fix the absolute path added on Sankore 3.1 1.00.00
    //The absoult path doesn't work when you want to share Sankore documents.
    if(!href.startsWith("videos/")){
        int indexOfAudioDirectory = href.lastIndexOf("videos");
        href = mDocumentPath + "/" + href.right(href.length() - indexOfAudioDirectory);
    }

    UBGraphicsMediaItem* videoItem = new UBGraphicsMediaItem(QUrl::fromLocalFile(href));
    if(videoItem){
        videoItem->connect(UBApplication::boardController, SIGNAL(activeSceneChanged()), videoItem, SLOT(activeSceneChanged()));
    }

    graphicsItemFromSvg(videoItem);
    QStringRef ubPos = mXmlReader.attributes().value(mNamespaceUri, "position");

    qint64 p = 0;
    if (!ubPos.isNull())
    {
        p = ubPos.toString().toLongLong();
    }

    videoItem->setInitialPos(p);
    return videoItem;
}

void UBSvgSubsetAdaptor::UBSvgSubsetReader::graphicsItemFromSvg(QGraphicsItem* gItem)
{

    QStringRef svgTransform = mXmlReader.attributes().value("transform");

    QMatrix itemMatrix;

    if (!svgTransform.isNull())
    {
        itemMatrix = fromSvgTransform(svgTransform.toString());
        gItem->setMatrix(itemMatrix);
    }

    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");

    if (mFileVersion >= 40202)
    {
        gItem->setPos(0,0);
    }
    else if (mFileVersion >= 40201)
    {
        if (!svgX.isNull() && !svgY.isNull())
        {
            gItem->setPos(svgX.toString().toFloat() * itemMatrix.m11(), svgY.toString().toFloat() * itemMatrix.m22());
        }
    }
    else
    {
        if (!svgX.isNull() && !svgY.isNull())
        {
            #ifdef Q_WS_WIN
                gItem->setPos(svgX.toString().toFloat(), svgY.toString().toFloat());
            #endif
        }
    }

    UBResizableGraphicsItem *rgi = dynamic_cast<UBResizableGraphicsItem*>(gItem);

    if (rgi)
    {
        QStringRef svgWidth = mXmlReader.attributes().value("width");
        QStringRef svgHeight = mXmlReader.attributes().value("height");

        if (!svgWidth.isNull() && !svgHeight.isNull())
        {
            rgi->resize(svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
        }
    }

    QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");

    if (!ubZValue.isNull())
    {
        UBGraphicsItem::assignZValue(gItem, ubZValue.toString().toFloat());
    }

    UBItem* ubItem = dynamic_cast<UBItem*>(gItem);

    if (ubItem)
    {
        QStringRef ubUuid = mXmlReader.attributes().value(mNamespaceUri, "uuid");

        if (!ubUuid.isNull())
        {
            ubItem->setUuid(QUuid(ubUuid.toString()));
        }

        QStringRef ubSource = mXmlReader.attributes().value(mNamespaceUri, "source");

        if (!ubSource.isNull())
        {
            ubItem->setSourceUrl(QUrl(ubSource.toString()));
        }
    }

    QStringRef ubLocked = mXmlReader.attributes().value(mNamespaceUri, "locked");

    if (!ubLocked.isNull())
    {
        bool isLocked = (ubLocked.toString() == xmlTrue || ubLocked.toString() == "1");
        gItem->setData(UBGraphicsItemData::ItemLocked, QVariant(isLocked));
    }

    QStringRef ubEditable = mXmlReader.attributes().value(mNamespaceUri, "editable");

    if (!ubEditable.isNull())
    {
        bool isEditable = (ubEditable.toString() == xmlTrue || ubEditable.toString() == "1");
        gItem->setData(UBGraphicsItemData::ItemEditable, QVariant(isEditable));
    }

    //deprecated as of 4.4.a.12
    QStringRef ubLayer = mXmlReader.attributes().value(mNamespaceUri, "layer");
    if (!ubLayer.isNull())
    {
        bool ok;
        int layerAsInt = ubLayer.toString().toInt(&ok);

        if (ok)
        {
            gItem->setData(UBGraphicsItemData::ItemLayerType, QVariant(layerAsInt));
        }
    }
}

qreal UBSvgSubsetAdaptor::UBSvgSubsetReader::getZValueFromSvg()
{
    qreal result = UBZLayerController::errorNum();

    QStringRef ubZValue = mXmlReader.attributes().value(mNamespaceUri, "z-value");
    if (!ubZValue.isNull()) {
        result  = ubZValue.toString().toFloat();
    }

    return result;
}

QUuid UBSvgSubsetAdaptor::UBSvgSubsetReader::getUuidFromSvg()
{
    QUuid result;

    QString strUuid = mXmlReader.attributes().value(mNamespaceUri, "uuid").toString();
    QUuid uuid = QUuid(strUuid);
    if (!uuid.isNull()) {
        result = uuid;
    }

    return result;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::graphicsItemToSvg(QGraphicsItem* item)
{
    mXmlWriter.writeAttribute("x", "0");
    mXmlWriter.writeAttribute("y", "0");

    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));

    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneMatrix()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    bool isBackground = mScene->isBackgroundObject(item);
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "background", isBackground ? xmlTrue : xmlFalse);

    // Issue 1684 - ALTI/AOU - 20131210
    if (isBackground)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "disposition",  QString::number(static_cast<int>(mScene->backgroundObjectDisposition())));
    }
    // Fin Issue 1684 - ALTI/AOU - 20131210

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));

        QUrl sourceUrl = ubItem->sourceUrl();

        if (!sourceUrl.isEmpty())
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "source", sourceUrl.toString());

    }

    QVariant layer = item->data(UBGraphicsItemData::ItemLayerType);
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "layer", QString("%1").arg(layer.toInt()));

    QVariant locked = item->data(UBGraphicsItemData::ItemLocked);

    if (!locked.isNull() && locked.toBool())
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "locked", xmlTrue);

    QVariant editable = item->data(UBGraphicsItemData::ItemEditable);
    if (!editable.isNull()) {
        if (editable.toBool())
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "editable", xmlTrue);
        else
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "editable", xmlFalse);
    }
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::graphicsAppleWidgetToSvg(UBGraphicsAppleWidgetItem* item)
{
    graphicsWidgetToSvg(item);
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::graphicsW3CWidgetToSvg(UBGraphicsW3CWidgetItem* item)
{
    graphicsWidgetToSvg(item);
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::graphicsWidgetToSvg(UBGraphicsWidgetItem* item)
{
    QUrl widgetRootUrl = item->widgetUrl();
    QString uuid = UBStringUtils::toCanonicalUuid(item->uuid());
    QString widgetDirectoryPath = UBPersistenceManager::widgetDirectory;
    if (widgetRootUrl.toString().startsWith("file://"))
    {
        QString widgetRootDir = widgetRootUrl.toLocalFile();
        QFileInfo fi(widgetRootDir);
        QString extension = fi.suffix();

        QString widgetTargetDir = widgetDirectoryPath + "/" + item->uuid().toString() + "." + extension;

        QString path = mDocumentPath + "/" + widgetTargetDir;
        QDir dir(path);

        if (!dir.exists(path))
        {
            QDir dir;
            dir.mkpath(path);
            UBFileSystemUtils::copyDir(widgetRootDir, path);
        }

        widgetRootUrl = widgetTargetDir;
    }

    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "src", widgetRootUrl.toString());


    if (item->isFeatureRTE())
        item->resize(item->size().width(), item->size().height()+UBGraphicsWidgetItem::sRTEEditionBarHeight);

    graphicsItemToSvg(item);

    if (item->isFrozen())
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "frozen", xmlTrue);
    }

    QString snapshotPath = mDocumentPath + "/" + UBPersistenceManager::widgetDirectory + "/" + uuid + ".png";
    item->snapshot().save(snapshotPath, "PNG");

    mXmlWriter.writeStartElement(nsXHtml, "iframe");

    mXmlWriter.writeAttribute("style", "border: none");
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));

    if (item->isFeatureRTE())
        item->resize(item->size().width(), item->size().height()-UBGraphicsWidgetItem::sRTEEditionBarHeight);

    QString startFileUrl;
    if (item->mainHtmlFileName().startsWith("http://"))
        startFileUrl = item->mainHtmlFileName();
    else
        startFileUrl = widgetRootUrl.toString() + "/" + item->mainHtmlFileName();

    mXmlWriter.writeAttribute("src", startFileUrl);
    mXmlWriter.writeEndElement(); //iFrame

    //persists widget state
    QMap<QString, QString> preferences = item->preferences();

    foreach(QString key, preferences.keys())
    {
        QString value = preferences.value(key);

        mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "preference");

        mXmlWriter.writeAttribute("key", key);
        mXmlWriter.writeAttribute("value", value);

        mXmlWriter.writeEndElement(); //ub::preference
    }

    //persists datasore state
    QMap<QString, QString> datastore = item->datastoreEntries();

    foreach(QString key, datastore.keys())
    {
        QString value = datastore.value(key);

        mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "datastoreEntry");

        mXmlWriter.writeAttribute("key", key);
        mXmlWriter.writeAttribute("value", value);

        mXmlWriter.writeEndElement(); //ub::datastoreEntry
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsAppleWidgetItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::graphicsAppleWidgetFromSvg()
{

    QStringRef widgetUrl = mXmlReader.attributes().value(mNamespaceUri, "src");

    if (widgetUrl.isNull())
    {
        qWarning() << "cannot make sens of widget src value";
        return 0;
    }

    QString href = widgetUrl.toString();

    QUrl url(href);

    if (url.isRelative())
    {
        href = mDocumentPath + "/" + UBFileSystemUtils::normalizeFilePath(widgetUrl.toString());
    }

    UBGraphicsAppleWidgetItem* widgetItem = new UBGraphicsAppleWidgetItem(QUrl::fromLocalFile(href));

    graphicsItemFromSvg(widgetItem);

    return widgetItem;
}

UBGraphicsW3CWidgetItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::graphicsW3CWidgetFromSvg()
{
    QStringRef widgetUrl = mXmlReader.attributes().value(mNamespaceUri, "src");

    if (widgetUrl.isNull())
    {
        qWarning() << "cannot make sens of widget src value";
        return 0;
    }

    QString href = widgetUrl.toString();
    QUrl url(href);

    if (url.isRelative())
    {
        href = mDocumentPath + "/" + UBFileSystemUtils::normalizeFilePath(widgetUrl.toString());
    }

    UBGraphicsW3CWidgetItem* widgetItem = new UBGraphicsW3CWidgetItem(QUrl::fromLocalFile(href));

    QStringRef uuid = mXmlReader.attributes().value(mNamespaceUri, "uuid");
    QString pixPath = mDocumentPath + "/" + UBPersistenceManager::widgetDirectory + "/" + uuid.toString() + ".png";

    QPixmap snapshot(pixPath);
    if (!snapshot.isNull())
        widgetItem->setSnapshot(snapshot);

    QStringRef frozen = mXmlReader.attributes().value(mNamespaceUri, "frozen");

    if (!frozen.isNull() && frozen.toString() == xmlTrue && !snapshot.isNull())
    {
        widgetItem->freeze();
    }

    graphicsItemFromSvg(widgetItem);

    return widgetItem;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::textItemToSvg(UBGraphicsTextItem* item)
{
    mXmlWriter.writeStartElement("foreignObject");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "type", "text");

    writeAction(item->Delegate()->action());

    graphicsItemToSvg(item);

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "width", QString("%1").arg(item->textWidth()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "height", QString("%1").arg(item->textHeight()));

    if(item->backgroundColor() != Qt::transparent)
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "backgroundColor", item->backgroundColor().name());

    QColor colorDarkBg = item->colorOnDarkBackground();
    QColor colorLightBg = item->colorOnLightBackground();

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                              , "fill-on-dark-background", colorDarkBg.name());
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri
                              , "fill-on-light-background", colorLightBg.name());

    //for new documents from version 4.5.0
    mXmlWriter.writeStartElement("itemTextContent");

    // Note: don't use mXmlWriter.writeCDATA(htmlString); because it doesn't escape characters sequences correctly.
    // Texts copied from other programs like Open-Office can truncate the svg file.
    //mXmlWriter.writeCharacters(item->toHtml());

    if(item->htmlMode())
        item->changeHTMLMode();

    QString content = UBTextTools::cleanHtmlCData(item->toHtml());

    if(mIsOldVersionFileWithText){
        content = content.replace(QRegExp("span style=\".*font-size:.*pt;\""), "span");
    }

    mXmlWriter.writeCharacters(content);

    mXmlWriter.writeEndElement(); //itemTextContent

    mXmlWriter.writeEndElement(); //foreignObject
}

UBGraphicsTextItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::textItemFromSvg()
{
    qreal width = mXmlReader.attributes().value("width").toString().toFloat();
    qreal height = mXmlReader.attributes().value("height").toString().toFloat();

    UBGraphicsTextItem* textItem = new UBGraphicsTextItem();
    graphicsItemFromSvg(textItem);

    //N/C - NNE - 20141211 : when importing a textItem, disable the text editor
    textItem->activateTextEditor(false);

    textItem->Delegate()->setAction(readAction());

    QStringRef ubFillOnDarkBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-dark-background");
    QStringRef ubFillOnLightBackground = mXmlReader.attributes().value(mNamespaceUri, "fill-on-light-background");

    if (!ubFillOnDarkBackground.isNull()) {
        QColor color;
        color.setNamedColor(ubFillOnDarkBackground.toString());
        if (!color.isValid())
            color = Qt::white;
        textItem->setColorOnDarkBackground(color);
    }

    if (!ubFillOnLightBackground.isNull()) {
        QColor color;
        color.setNamedColor(ubFillOnLightBackground.toString());
        if (!color.isValid())
            color = Qt::black;
            textItem->setColorOnLightBackground(color);
    }

    QStringRef ubBackgroundColor = mXmlReader.attributes().value(mNamespaceUri, "backgroundColor");

    if(!ubBackgroundColor.isNull()){
        QColor color;
        color.setNamedColor(ubBackgroundColor.toString());

        if (color.isValid())
            textItem->setBackgroundColor(color);
    }

    QString text;

    while (!(mXmlReader.isEndElement() && (mXmlReader.name() == "font" || mXmlReader.name() == "foreignObject")))
    {
        if (mXmlReader.hasError())
        {
            delete textItem;
            return 0;
        }

        mXmlReader.readNext();
        if (mXmlReader.isStartElement())
        {
            //for new documents from version 4.5.0
            if (mFileVersion >= 40500) {
                if (mXmlReader.name() == "itemTextContent") {
                    text = mXmlReader.readElementText();

                    textItem->setHtml(text);

                    textItem->resize(width, height);

                    if (textItem->toPlainText().isEmpty()) {
                        delete textItem;
                        textItem = 0;
                    }
                    else
                    {
                        //CFA - add resources like images
                        QDomDocument html;
                        html.setContent(text);

                        QDomNodeList imgNodes = html.elementsByTagName("img");

                        if (imgNodes.count()>0)
                        {
                            for (int i =0; i < imgNodes.count(); i++)
                            {
                                QString filename = imgNodes.item(i).toElement().attribute("src");
                                QString dest = mDocumentPath + "/" + filename;
                                textItem->document()->addResource(QTextDocument::ImageResource, QUrl(filename), QImage(dest));
                                textItem->adjustSize();
                            }
                        }
                    }
                    return textItem;
                }

            //tracking for backward capability with older versions
            }
            else if (mXmlReader.name() == "font")  {
                mIsOldVersionFileWithText = true;
                QFont font = textItem->font();

                QStringRef fontFamily = mXmlReader.attributes().value("face");

                if (!fontFamily.isNull()) {
                    font.setFamily(fontFamily.toString());
                }
                QStringRef fontStyle = mXmlReader.attributes().value("style");
                if (!fontStyle.isNull()) {
                    foreach (QString styleToken, fontStyle.toString().split(";")) {
                        styleToken = styleToken.trimmed();
                        if (styleToken.startsWith(sFontSizePrefix) && styleToken.endsWith(sPixelUnit)) {
                            int fontSize = styleToken.mid(
                                               sFontSizePrefix.length(),
                                               styleToken.length() - sFontSizePrefix.length() - sPixelUnit.length()).toInt();
                            font.setPixelSize(fontSize);
                        } else if (styleToken.startsWith(sFontWeightPrefix)) {
                            QString fontWeight = styleToken.mid(
                                                     sFontWeightPrefix.length(),
                                                     styleToken.length() - sFontWeightPrefix.length());
                            font.setBold(fontWeight.contains("bold"));
                        } else if (styleToken.startsWith(sFontStylePrefix)) {
                            QString fontStyle = styleToken.mid(
                                                    sFontStylePrefix.length(),
                                                    styleToken.length() - sFontStylePrefix.length());
                            font.setItalic(fontStyle.contains("italic"));
                        }
                    }
                }

                QTextCursor curCursor = textItem->textCursor();
                QTextCharFormat format;

                format.setFont(font);
                curCursor.mergeCharFormat(format);
                textItem->setTextCursor(curCursor);
                textItem->setFont(font);

                QStringRef fill = mXmlReader.attributes().value("color");
                if (!fill.isNull()) {
                    QColor textColor;
                    textColor.setNamedColor(fill.toString());
                    textItem->setDefaultTextColor(textColor);
                }

                while (!(mXmlReader.isEndElement() && mXmlReader.name() == "font")) {
                    if (mXmlReader.hasError()) {
                        break;
                    }
                    QXmlStreamReader::TokenType tt = mXmlReader.readNext();
                    if (tt == QXmlStreamReader::Characters) {
                        text += mXmlReader.text().toString();
                    }

                    if (mXmlReader.isStartElement() && mXmlReader.name() == "br") {
                        text += "\n";
                    }
                }
            }
        }
    }


    if (text.isEmpty()) {
        delete textItem;
        textItem = 0;
    }
    else {
        textItem->setPlainText(text);
        textItem->resize(width, height);
    }

    return textItem;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::curtainItemToSvg(UBGraphicsCurtainItem* curtainItem)
{
    /**
     *
     * sample
     *
      <ub:curtain x="250" y="150" width="122" height="67"...>
      </ub:curtain>
     */

    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "curtain");
    mXmlWriter.writeAttribute("x", QString("%1").arg(curtainItem->boundingRect().center().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(curtainItem->boundingRect().center().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(curtainItem->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(curtainItem->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(curtainItem->sceneMatrix()));

    //graphicsItemToSvg(curtainItem);
    QString zs;
    zs.setNum(curtainItem->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(curtainItem);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsCurtainItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::curtainItemFromSvg()
{
    UBGraphicsCurtainItem* curtainItem = new UBGraphicsCurtainItem();

    graphicsItemFromSvg(curtainItem);

    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgWidth = mXmlReader.attributes().value("width");
    QStringRef svgHeight = mXmlReader.attributes().value("height");


    QRect rect;
    rect.setX(svgX.toString().toFloat()-svgWidth.toString().toFloat()/2);
    rect.setY(svgY.toString().toFloat()-svgHeight.toString().toFloat()/2);
    rect.setWidth(svgWidth.toString().toFloat());
    rect.setHeight(svgHeight.toString().toFloat());

    curtainItem->setRect(rect);

    curtainItem->setVisible(true);

    return curtainItem;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::rulerToSvg(UBGraphicsRuler* item)
{

    /**
     *
     * sample
     *
      <ub:ruler x="250" y="150" width="122" height="67"...>
      </ub:ruler>
     */

    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "ruler");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneMatrix()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsRuler* UBSvgSubsetAdaptor::UBSvgSubsetReader::rulerFromSvg()
{
    UBGraphicsRuler* ruler = new UBGraphicsRuler();

    graphicsItemFromSvg(ruler);

    ruler->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Tool));

    QStringRef svgWidth = mXmlReader.attributes().value("width");
    QStringRef svgHeight = mXmlReader.attributes().value("height");
    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");

    if (!svgWidth.isNull() && !svgHeight.isNull() && !svgX.isNull() && !svgY.isNull())
    {
        ruler->setRect(svgX.toString().toFloat(), svgY.toString().toFloat(),  svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    ruler->setVisible(true);

    return ruler;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::compassToSvg(UBGraphicsCompass* item)
{

    /**
     *
     * sample
     *
      <ub:compass x="250" y="150" width="122" height="67"...>
      </ub:compass>
     */

    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "compass");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneMatrix()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsCompass* UBSvgSubsetAdaptor::UBSvgSubsetReader::compassFromSvg()
{
    UBGraphicsCompass* compass = new UBGraphicsCompass();

    graphicsItemFromSvg(compass);

    compass->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Tool));

    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgWidth = mXmlReader.attributes().value("width");
    QStringRef svgHeight = mXmlReader.attributes().value("height");

    if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        compass->setRect(svgX.toString().toFloat(), svgY.toString().toFloat()
                         , svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    compass->setVisible(true);

    return compass;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::protractorToSvg(UBGraphicsProtractor* item)
{

    /**
     *
     * sample
     *
      <ub:protractor x="250" y="150" width="122" height="67"...>
      </ub:protractor>
     */

    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "protractor");

    mXmlWriter.writeAttribute("x", QString("%1").arg(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->rect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneMatrix()));

    QString angle;
    angle.setNum(item->angle(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "angle", angle);
    angle.setNum(item->markerAngle(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "marker-angle", angle);

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsProtractor* UBSvgSubsetAdaptor::UBSvgSubsetReader::protractorFromSvg()
{
    UBGraphicsProtractor* protractor = new UBGraphicsProtractor();

    protractor->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Tool));

    graphicsItemFromSvg(protractor);

    QStringRef angle = mXmlReader.attributes().value(mNamespaceUri, "angle");
    if (!angle.isNull())
    {
        protractor->setAngle(angle.toString().toFloat());
    }

    QStringRef markerAngle = mXmlReader.attributes().value(mNamespaceUri, "marker-angle");
    if (!markerAngle.isNull())
    {
        protractor->setMarkerAngle(markerAngle.toString().toFloat());
    }

    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgWidth = mXmlReader.attributes().value("width");
    QStringRef svgHeight = mXmlReader.attributes().value("height");

    if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        protractor->setRect(svgX.toString().toFloat(), svgY.toString().toFloat()
                            , svgWidth.toString().toFloat(), svgHeight.toString().toFloat());
    }

    protractor->setVisible(true);

    return protractor;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::triangleToSvg(UBGraphicsTriangle *item)
{

    /**
     *
     * sample
     *
      <ub:triangle x="250" y="150" width="122" height="67"...>
      </ub:triangle>
     */

    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "triangle");
    mXmlWriter.writeAttribute("x", QString("%1").arg(item->boundingRect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->boundingRect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->boundingRect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->boundingRect().height()));
    mXmlWriter.writeAttribute("transform", toSvgTransform(item->sceneMatrix()));
    mXmlWriter.writeAttribute("orientation", UBGraphicsTriangle::orientationToStr(item->getOrientation()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UBGraphicsTriangle* UBSvgSubsetAdaptor::UBSvgSubsetReader::triangleFromSvg()
{
    UBGraphicsTriangle* triangle = new UBGraphicsTriangle();

    triangle->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Tool));

    graphicsItemFromSvg(triangle);

    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgWidth = mXmlReader.attributes().value("width");
    QStringRef svgHeight = mXmlReader.attributes().value("height");

    QStringRef orientationStringRef = mXmlReader.attributes().value("orientation");
    UBGraphicsTriangle::UBGraphicsTriangleOrientation orientation = UBGraphicsTriangle::orientationFromStr(orientationStringRef);
    triangle->setOrientation(orientation);

        if (!svgX.isNull() && !svgY.isNull() && !svgWidth.isNull() && !svgHeight.isNull())
    {
        triangle->setRect(svgX.toString().toFloat(), svgY.toString().toFloat(), svgWidth.toString().toFloat(), svgHeight.toString().toFloat(), orientation);
    }

    triangle->setVisible(true);
    return triangle;
}

UBGraphicsCache* UBSvgSubsetAdaptor::UBSvgSubsetReader::cacheFromSvg()
{
    UBGraphicsCache* pCache = UBGraphicsCache::instance(mScene);
    pCache->setData(UBGraphicsItemData::ItemLayerType, QVariant(UBItemLayerType::Tool));

    graphicsItemFromSvg(pCache);

    QStringRef colorR = mXmlReader.attributes().value("colorR");
    QStringRef colorG = mXmlReader.attributes().value("colorG");
    QStringRef colorB = mXmlReader.attributes().value("colorB");
    QStringRef colorA = mXmlReader.attributes().value("colorA");
    QStringRef shape = mXmlReader.attributes().value("shape");
    QStringRef shapeSize = mXmlReader.attributes().value("shapeSize");

    QColor color(colorR.toString().toInt(), colorG.toString().toInt(), colorB.toString().toInt(), colorA.toString().toInt());

    pCache->setMaskColor(color);
    pCache->setHoleWidth(shapeSize.toString().toInt());
    pCache->setMaskShape(static_cast<eMaskShape>(shape.toString().toInt()));

    pCache->setVisible(true);

    return pCache;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::cacheToSvg(UBGraphicsCache* item)
{
    mXmlWriter.writeStartElement(UBSettings::uniboardDocumentNamespaceUri, "cache");

    mXmlWriter.writeAttribute("x", QString("%1").arg(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString("%1").arg(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString("%1").arg(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString("%1").arg(item->rect().height()));
    mXmlWriter.writeAttribute("colorR", QString("%1").arg(item->maskColor().red()));
    mXmlWriter.writeAttribute("colorG", QString("%1").arg(item->maskColor().green()));
    mXmlWriter.writeAttribute("colorB", QString("%1").arg(item->maskColor().blue()));
    mXmlWriter.writeAttribute("colorA", QString("%1").arg(item->maskColor().alpha()));
    mXmlWriter.writeAttribute("shape", QString("%1").arg(item->maskshape()));
    mXmlWriter.writeAttribute("shapeSize", QString("%1").arg(item->holeWidth()));

    QString zs;
    zs.setNum(item->zValue(), 'f'); // 'f' keeps precision
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "z-value", zs);

    UBItem* ubItem = dynamic_cast<UBItem*>(item);

    if (ubItem)
    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(ubItem->uuid()));
    }

    mXmlWriter.writeEndElement();
}

UB3HEditableGraphicsRectItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapeRectFromSvg(const QColor& pDefaultPenColor) // EV-7 - ALTI/AOU - 20131231
{
    UB3HEditableGraphicsRectItem* rect = new UB3HEditableGraphicsRectItem();

    qreal x=0, y=0, w=100, h=100;
    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgW = mXmlReader.attributes().value("width");
    QStringRef svgH = mXmlReader.attributes().value("height");

    if ( ! svgX.isNull()) x = svgX.toString().toFloat();
    if ( ! svgY.isNull()) y = svgY.toString().toFloat();
    if ( ! svgW.isNull()) w = svgW.toString().toFloat();
    if ( ! svgH.isNull()) h = svgH.toString().toFloat();

    rect->setRect(QRectF(0, 0, w, h));

    getStyleFromSvg(rect, pDefaultPenColor);

    return rect;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapeRectToSvg(UB3HEditableGraphicsRectItem *item) // EV-7 - ALTI/AOU - 20131231
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("rect");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapeRect", "true");

    // SVG <shapeRect> tag :
    mXmlWriter.writeAttribute("x", QString::number(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString::number(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString::number(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString::number(item->rect().height()));

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();
}

QLinearGradient UBSvgSubsetAdaptor::UBSvgSubsetReader::linearGradiantFromSvg()
{
    QLinearGradient gradient;

    mXmlReader.readNext();
    mXmlReader.readNext();//<stop>
    QString stop0 = mXmlReader.attributes().value("style").toString();

    QStringList split = stop0.split(":");
    QStringList rgb = split.at(1).split(";");
    QStringList colors = rgb.at(0).split(",");
    int color1R = colors.at(0).split("(").at(1).toInt();
    int color1G = colors.at(1).toInt();
    int color1B = colors.at(2).split(")").at(0).toInt();

    qreal alphaF = split.at(2).toFloat();

    QColor color1(color1R, color1G, color1B);
    color1.setAlphaF(alphaF);
    gradient.setColorAt(0, color1);

    mXmlReader.readNext();
    mXmlReader.readNext();
    mXmlReader.readNext();//<stop>

    QString stop1 =  mXmlReader.attributes().value("style").toString();

    split = stop1.split(":");
    rgb = split.at(1).split(";");
    colors = rgb.at(0).split(",");
    int color2R = colors.at(0).split("(").at(1).toInt();
    int color2G = colors.at(1).toInt();
    int color2B = colors.at(2).split(")").at(0).toInt();

    alphaF = split.at(2).toFloat();

    QColor color2(color2R, color2G, color2B);
    color2.setAlphaF(alphaF);
    gradient.setColorAt(1, color2);

    return gradient;

}

void UBSvgSubsetAdaptor::UBSvgSubsetReader::getStyleFromSvg(UBAbstractGraphicsItem *item, const QColor &pDefaultPenColor)
{
    QPen p = item->pen();
    // Stroke color
    QStringRef svgStroke = mXmlReader.attributes().value("stroke");
    QColor strokeColor = pDefaultPenColor;
    if (!svgStroke.isNull())
    {
        strokeColor.setNamedColor(svgStroke.toString());
    }

    QStringRef svgStrokeOpacity = mXmlReader.attributes().value("stroke-opacity");
    if (!svgStrokeOpacity.isNull())
    {
        strokeColor.setAlphaF(svgStrokeOpacity.toString().toInt());
    }

    p.setColor(strokeColor);

    // Stroke width/thickness
    QStringRef svgStrokeWidth = mXmlReader.attributes().value("stroke-width");
    int strokeWidth = 2;
    if (!svgStrokeWidth.isNull())
    {
        strokeWidth = svgStrokeWidth.toString().toInt();
    }

    p.setWidth(strokeWidth);

    // Stroke style
    QStringRef svgStrokeLineCap = mXmlReader.attributes().value("stroke-linecap");

    if(!svgStrokeLineCap.isNull() && svgStrokeLineCap.toString().toLower() == "round"){
        //Custom dash line style
        p.setCapStyle(Qt::RoundCap);
        QVector<qreal> dashPattern = UBApplication::boardController->shapeFactory().dashPattern();
        p.setDashPattern(dashPattern);
    }else{
        QStringRef svgStrokeStyle = mXmlReader.attributes().value("stroke-dasharray");
        if (!svgStrokeStyle.isNull())
        {
            QString strokeStyle = svgStrokeStyle.toString();
            if (strokeStyle == SVG_STROKE_DOTLINE)
            {
                p.setStyle(Qt::DotLine);
            }
        }
    }


    item->setPen(p);

    QBrush b = item->brush();

    // Fill color
    QStringRef svgFill1 = mXmlReader.attributes().value("fill");
    QColor brushColor1 = Qt::transparent;
    if (!svgFill1.isNull())
    {
        brushColor1.setNamedColor(svgFill1.toString());
    }

    // Fill opacity (transparency)
    QStringRef svgOpacity1 = mXmlReader.attributes().value("fill-opacity");
    qreal opacity1 = 1.0; // opaque
    if (!svgOpacity1.isNull())
    {
        opacity1 = svgOpacity1.toString().toFloat();
    }

    if (!item->hasGradient())
    {
        brushColor1.setAlphaF(opacity1);
    }

    QStringRef style = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "fill-style");
    b.setStyle(static_cast<Qt::BrushStyle>(style.toString().toInt()));
    b.setColor(brushColor1);

    item->setBrush(b);

    // Fill pattern
    QStringRef svgFillPattern = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "fill-pattern");
    UBAbstractGraphicsItem::FillPattern fillPattern = UBAbstractGraphicsItem::FillPattern_None;
    if (!svgFillPattern.isNull())
    {
        fillPattern = static_cast<UBAbstractGraphicsItem::FillPattern>(svgFillPattern.toString().toInt());
    }
    item->setFillPattern(fillPattern);

    // action
    item->Delegate()->setAction(readAction());

    // Transform matrix
    QStringRef svgTransform = mXmlReader.attributes().value("transform");
    QMatrix itemMatrix;
    if (!svgTransform.isNull())
    {
        itemMatrix = fromSvgTransform(svgTransform.toString());
        item->setMatrix(itemMatrix);
    }
}

UB3HEditableGraphicsEllipseItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapeEllipseFromSvg(const QColor& pDefaultPenColor) // EV-7 - ALTI/AOU - 20131231
{
    UB3HEditableGraphicsEllipseItem * ellipse = new UB3HEditableGraphicsEllipseItem();

    qreal cx=0, cy=0, rx=100, ry=100;
    QStringRef svgCx = mXmlReader.attributes().value("cx");
    QStringRef svgCy = mXmlReader.attributes().value("cy");
    QStringRef svgRx = mXmlReader.attributes().value("rx");
    QStringRef svgRy = mXmlReader.attributes().value("ry");

    if ( ! svgCx.isNull()) cx = svgCx.toString().toFloat();
    if ( ! svgCy.isNull()) cy = svgCy.toString().toFloat();
    if ( ! svgRx.isNull()) rx = svgRx.toString().toFloat();
    if ( ! svgRy.isNull()) ry = svgRy.toString().toFloat();

    ellipse->setRect(QRectF(0, 0, 2*rx, 2*ry));

    getStyleFromSvg(ellipse, pDefaultPenColor);

    return ellipse;
}

UB1HEditableGraphicsCircleItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapeCircleFromSvg(const QColor &pDefaultPenColor)
{
    UB1HEditableGraphicsCircleItem * circle = new UB1HEditableGraphicsCircleItem();

    qreal cx=0, cy=0, rx=100;
    QStringRef svgCx = mXmlReader.attributes().value("cx");
    QStringRef svgCy = mXmlReader.attributes().value("cy");
    QStringRef svgRx = mXmlReader.attributes().value("rx");

    if ( ! svgCx.isNull()) cx = svgCx.toString().toFloat();
    if ( ! svgCy.isNull()) cy = svgCy.toString().toFloat();
    if ( ! svgRx.isNull()) rx = svgRx.toString().toFloat();


    circle->setRect(QRectF(0, 0, 2*rx, 2*rx));

    getStyleFromSvg(circle, pDefaultPenColor);

    return circle;
}

UB1HEditableGraphicsSquareItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapeSquareFromSvg(const QColor &pDefaultPenColor)
{
    UB1HEditableGraphicsSquareItem* square = new UB1HEditableGraphicsSquareItem();

    qreal x=0, y=0, side = 0;
    QStringRef svgX = mXmlReader.attributes().value("x");
    QStringRef svgY = mXmlReader.attributes().value("y");
    QStringRef svgS = mXmlReader.attributes().value("width");

    if ( ! svgX.isNull()) x = svgX.toString().toFloat();
    if ( ! svgY.isNull()) y = svgY.toString().toFloat();
    if ( ! svgS.isNull()) side = svgS.toString().toFloat();

    square->setRect(QRectF(0, 0, side, side));

    getStyleFromSvg(square, pDefaultPenColor);

    return square;
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapeEllipseToSvg(UB3HEditableGraphicsEllipseItem *item) // EV-7 - ALTI/AOU - 20131231
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("ellipse");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapeEllipse", "true");

    // SVG <ellipse> tag :
    mXmlWriter.writeAttribute("cx", QString("%1").arg(item->center().x())); // The <ellipse> SVG tag need center coordinates. Compute them from boundaries of item.
    mXmlWriter.writeAttribute("cy", QString("%1").arg(item->center().y()));
    mXmlWriter.writeAttribute("rx", QString("%1").arg(item->radiusX()));
    mXmlWriter.writeAttribute("ry", QString("%1").arg(item->radiusY()));

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();
}

UBAbstractGraphicsPathItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapePathFromSvg(const QColor& pDefaultPenColor, int type) // EV-7 - ALTI/AOU - 20140102
{
    UBAbstractGraphicsPathItem *pathItem = 0;

    switch(type){
    case UBGraphicsFreehandItem::Type:
        pathItem = new UBGraphicsFreehandItem();
        break;
    case UBEditableGraphicsPolygonItem::Type:
        pathItem = new UBEditableGraphicsPolygonItem();
        break;
    case UBEditableGraphicsLineItem::Type:
        pathItem = new UBEditableGraphicsLineItem();
        break;
    default:
        break;
    }

    QStringRef svgPoints = mXmlReader.attributes().value("points");

    if (pathItem && !svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '),
                         QString::SkipEmptyParts);

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), QString::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                pathItem->addPoint(point);
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                pathItem->addPoint(point);
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }            
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    // Arrows on extremities :
    QStringRef startArrowType = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "startArrowType");
    if (!startArrowType.isNull()) {
        pathItem->setStartArrowType((UBAbstractGraphicsPathItem::ArrowType)startArrowType.toString().toInt());
    }
    QStringRef endArrowType = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "endArrowType");
    if (!endArrowType.isNull()) {
        pathItem->setEndArrowType((UBAbstractGraphicsPathItem::ArrowType)endArrowType.toString().toInt());
    }

    getStyleFromSvg(pathItem, pDefaultPenColor);

    return pathItem;
}

UBEditableGraphicsRegularShapeItem* UBSvgSubsetAdaptor::UBSvgSubsetReader::shapeRegularFromSvg(const QColor& pDefaultPenColor)
{
    QStringRef svgPoints = mXmlReader.attributes().value("points");

    QStringRef nVertices = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "nVertices");
    QStringRef startPointX = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "startPointX");
    QStringRef startPointY = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "startPointY");

    QStringRef cCenterX = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "cCenterX");
    QStringRef cCenterY = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "cCenterY");
    QStringRef cRadius = mXmlReader.attributes().value(UBSettings::uniboardDocumentNamespaceUri, "cRadius");

    UBEditableGraphicsRegularShapeItem *pathItem = new UBEditableGraphicsRegularShapeItem(nVertices.toString().toInt(), QPointF(startPointX.toString().toFloat(), startPointY.toString().toFloat()));

    pathItem->setCircumscribedCenterCircle(QPointF(cCenterX.toString().toFloat(), cCenterY.toString().toFloat()));
    pathItem->setCircumscribedRadiusCircle(cRadius.toString().toFloat());

    if (!svgPoints.isNull())
    {
        QStringList ts = svgPoints.toString().split(QLatin1Char(' '),
                         QString::SkipEmptyParts);

        foreach(const QString sPoint, ts)
        {
            QStringList sCoord = sPoint.split(QLatin1Char(','), QString::SkipEmptyParts);

            if (sCoord.size() == 2)
            {
                QPointF point;
                point.setX(sCoord.at(0).toFloat());
                point.setY(sCoord.at(1).toFloat());
                pathItem->addPoint(point);
            }
            else if (sCoord.size() == 4){
                //This is the case on system were the "," is used to seperate decimal
                QPointF point;
                QString x = sCoord.at(0) + "." + sCoord.at(1);
                QString y = sCoord.at(2) + "." + sCoord.at(3);
                point.setX(x.toFloat());
                point.setY(y.toFloat());
                pathItem->addPoint(point);
            }
            else
            {
                qWarning() << "cannot make sense of a 'point' value" << sCoord;
            }
        }
    }
    else
    {
        qWarning() << "cannot make sense of 'points' value " << svgPoints.toString();
    }

    getStyleFromSvg(pathItem, pDefaultPenColor);

    return pathItem;
}


void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapeRegularToSvg(UBEditableGraphicsRegularShapeItem *item)
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("polyline");

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapePath", QString::number(item->type())); // just to know it's a path drawn with the drawingPalette

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "nVertices", QString::number(item->nVertices()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "startPointX", QString::number(item->startPoint().x()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "startPointY", QString::number(item->startPoint().y()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "cCenterX", QString::number(item->circumscribedCenterCircle().x()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "cCenterY", QString::number(item->circumscribedCenterCircle().y()));
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "cRadius", QString::number(item->circumscribedRadiusCircle()));

    QString sPoints;
    for(int i=0; i<item->path().elementCount(); ++i)
    {
        QPainterPath::Element element = item->path().elementAt(i);
        sPoints += QString("%1,%2 ").arg(element.x).arg(element.y);
    }

    mXmlWriter.writeAttribute("points", sPoints);

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapePathToSvg(UBAbstractGraphicsPathItem *item) // EV-7 - ALTI/AOU - 20140102
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("polyline");

    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapePath", QString::number(item->type())); // just to know it's a path drawn with the drawingPalette

    if(dynamic_cast<UBEditableGraphicsLineItem*>(item))
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "isLine", "true");

    QString sPoints;
    for(int i=0; i<item->path().elementCount(); ++i)
    {
        QPainterPath::Element element = item->path().elementAt(i);
        sPoints += QString("%1,%2 ").arg(element.x).arg(element.y);
    }

    mXmlWriter.writeAttribute("points", sPoints);

    // Arrows :
    UBAbstractGraphicsPathItem::ArrowType startArrowType = item->startArrowType();
    if (startArrowType != UBAbstractGraphicsPathItem::ArrowType_None){
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "startArrowType", QString::number(startArrowType));
    }

    UBAbstractGraphicsPathItem::ArrowType endArrowType = item->endArrowType();
    if (endArrowType != UBAbstractGraphicsPathItem::ArrowType_None)    {
        mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "endArrowType", QString::number(endArrowType));
    }

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();

}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapeSquareToSvg(UB1HEditableGraphicsSquareItem *item)
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("rect");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapeRect", "true");

    // SVG <shapeRect> tag :
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "isSquare", "true");
    mXmlWriter.writeAttribute("x", QString::number(item->rect().x()));
    mXmlWriter.writeAttribute("y", QString::number(item->rect().y()));
    mXmlWriter.writeAttribute("width", QString::number(item->rect().width()));
    mXmlWriter.writeAttribute("height", QString::number(item->rect().height()));

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::shapeCircleToSvg(UB1HEditableGraphicsCircleItem *item)
{
    writeAbstractGraphicsItemGradient(item);

    mXmlWriter.writeStartElement("ellipse");
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "shapeEllipse", "true");

    // SVG <ellipse> tag :
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "isCircle", "true");
    mXmlWriter.writeAttribute("cx", QString("%1").arg(item->center().x()));
    mXmlWriter.writeAttribute("cy", QString("%1").arg(item->center().y()));
    mXmlWriter.writeAttribute("rx", QString("%1").arg(item->radius()));
    mXmlWriter.writeAttribute("ry", QString("%1").arg(item->radius()));

    writeAbstractGraphicsItemStyle(item);

    mXmlWriter.writeEndElement();
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::writeAbstractGraphicsItemGradient(UBAbstractGraphicsItem *item)
{
    if(item->hasGradient()){
        QColor color1 = item->brush().gradient()->stops().at(0).second;
        QColor color2 = item->brush().gradient()->stops().at(1).second;
        mXmlWriter.writeStartElement("linearGradient");
        mXmlWriter.writeAttribute("x1", "0%");
        mXmlWriter.writeAttribute("y1", "0%");
        mXmlWriter.writeAttribute("x2", "100%");
        mXmlWriter.writeAttribute("y2", "0%");
        mXmlWriter.writeStartElement("stop");
        mXmlWriter.writeAttribute("offset", "0%");
        mXmlWriter.writeAttribute("style", QString("stop-color:rgb(%1,%2,%3);stop-opacity:%4").arg(QString::number(color1.red()), QString::number(color1.green()), QString::number(color1.blue()), QString::number(color1.alphaF())));
        mXmlWriter.writeEndElement();
        mXmlWriter.writeStartElement("stop");
        mXmlWriter.writeAttribute("offset", "100%");
        mXmlWriter.writeAttribute("style", QString("stop-color:rgb(%1,%2,%3);stop-opacity:%4").arg(QString::number(color2.red()), QString::number(color2.green()), QString::number(color2.blue()), QString::number(color2.alphaF())));
        mXmlWriter.writeEndElement();
        mXmlWriter.writeEndElement();
    }
}

void UBSvgSubsetAdaptor::UBSvgSubsetWriter::writeAbstractGraphicsItemStyle(UBAbstractGraphicsItem *item)
{
    // Stroke :
    if(item->hasStrokeProperty()){
        mXmlWriter.writeAttribute("stroke", QString("%1").arg(item->pen().color().name()));
        mXmlWriter.writeAttribute("stroke-width", QString("%1").arg(item->pen().width()));

        if (item->pen().style() == Qt::DotLine){
            mXmlWriter.writeAttribute("stroke-dasharray", SVG_STROKE_DOTLINE);
        }

        if(item->pen().style() == Qt::CustomDashLine){
            switch(item->pen().width()){
                case UBDrawingStrokePropertiesPalette::Fine:
                    mXmlWriter.writeAttribute("stroke-dasharray", "1, 10");
                    break;
                case UBDrawingStrokePropertiesPalette::Medium:
                    mXmlWriter.writeAttribute("stroke-dasharray", "1, 15");
                    break;
                case UBDrawingStrokePropertiesPalette::Large:
                    mXmlWriter.writeAttribute("stroke-dasharray", "1, 20");
                    break;
                default:
                    mXmlWriter.writeAttribute("stroke-dasharray", "1, 3");
            }

            mXmlWriter.writeAttribute("stroke-linecap", "round");
        }

        mXmlWriter.writeAttribute("stroke-opacity", QString("%1").arg(item->pen().color().alphaF()));
    }

    // Fill :
    if (item->hasFillingProperty())
    {
        if (!item->hasGradient())
        {
            mXmlWriter.writeAttribute("fill", QString("%1").arg(item->brush().color().name()));
            mXmlWriter.writeAttribute("fill-opacity", QString("%1").arg(item->brush().color().alphaF()));
            mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "fill-style", QString("%1").arg(item->brush().style()));

            if (item->brush().style() == Qt::TexturePattern){
                mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "fill-pattern", QString("%1").arg(item->fillPattern()));
            }
        }
        else
        {
            mXmlWriter.writeAttribute("fill", QString("url(#%1)").arg(item->uuid().toString()));
        }
    }
    else
        mXmlWriter.writeAttribute("fill", "none");

    //action
    writeAction(item->Delegate()->action());

    //uuid
    mXmlWriter.writeAttribute(UBSettings::uniboardDocumentNamespaceUri, "uuid", UBStringUtils::toCanonicalUuid(item->uuid()));

    //transform matrix
    mXmlWriter.writeAttribute("transform",toSvgTransform(item->sceneMatrix()));
}

void UBSvgSubsetAdaptor::convertPDFObjectsToImages(UBDocumentProxy* proxy)
{
    for (int i = 0; i < proxy->pageCount(); i++)
    {
        UBGraphicsScene* scene = loadScene(proxy, i);

        if (scene)
        {
            bool foundPDFItem = false;

            foreach(QGraphicsItem* item, scene->items())
            {
                UBGraphicsPDFItem *pdfItem = dynamic_cast<UBGraphicsPDFItem*>(item);

                if (pdfItem)
                {
                    foundPDFItem = true;
                    UBGraphicsPixmapItem* pixmapItem = pdfItem->toPixmapItem();

                    scene->removeItem(pdfItem);
                    scene->addItem(pixmapItem);

                }
            }

            if (foundPDFItem)
            {
                scene->setModified(true);
                persistScene(proxy, scene, i);
            }
        }

    }
}

void UBSvgSubsetAdaptor::convertSvgImagesToImages(UBDocumentProxy* proxy)
{
    for (int i = 0; i < proxy->pageCount(); i++)
    {
        UBGraphicsScene* scene = loadScene(proxy, i);

        if (scene)
        {
            bool foundSvgItem = false;

            foreach(QGraphicsItem* item, scene->items())
            {
                UBGraphicsSvgItem *svgItem = dynamic_cast<UBGraphicsSvgItem*>(item);

                if (svgItem)
                {
                    foundSvgItem = true;
                    UBGraphicsPixmapItem* pixmapItem = svgItem->toPixmapItem();

                    scene->removeItem(svgItem);
                    scene->addItem(pixmapItem);
                }
            }

            if (foundSvgItem)
            {
                scene->setModified(true);
                persistScene(proxy, scene, i);
            }
        }
    }
}






