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



#ifndef UBSVGSUBSETADAPTOR_H_
#define UBSVGSUBSETADAPTOR_H_

#include <QtGui>
#include <QtXml>

#include "frameworks/UBGeometryUtils.h"

class UBGraphicsSvgItem;
class UBGraphicsPolygonItem;
class UBGraphicsPixmapItem;
class UBGraphicsPDFItem;
class UBGraphicsWidgetItem;
class UBGraphicsMediaItem;
class UBGraphicsAppleWidgetItem;
class UBGraphicsW3CWidgetItem;
class UBGraphicsTextItem;
class UBGraphicsCurtainItem;
class UBGraphicsRuler;
class UBGraphicsCompass;
class UBGraphicsProtractor;
class UBGraphicsScene;
class UBDocumentProxy;
class UBGraphicsStroke;
class UBPersistenceManager;
class UBGraphicsTriangle;
class UBGraphicsCache;
class IDataStorage;
class UBGraphicsGroupContainerItem;
class UBGraphicsItemAction;
class UBGraphicsStrokesGroup;
class UB3HEditableGraphicsEllipseItem;
class UB3HEditableGraphicsRectItem;
class UBEditableGraphicsPolygonItem;
class UBAbstractGraphicsPathItem;
class UBEditableGraphicsLineItem;
class UBAbstractGraphicsItem;
class UB1HEditableGraphicsCircleItem;
class UB1HEditableGraphicsSquareItem;
class UBEditableGraphicsRegularShapeItem;

class UBSvgSubsetAdaptor
{
    private:

        UBSvgSubsetAdaptor() {;}
        virtual ~UBSvgSubsetAdaptor() {;}

    public:

        static UBGraphicsScene* loadScene(UBDocumentProxy* proxy, const int pageIndex);
        static void persistScene(UBDocumentProxy* proxy, UBGraphicsScene* pScene, const int pageIndex);
        static void upgradeScene(UBDocumentProxy* proxy, const int pageIndex);

        static QUuid sceneUuid(UBDocumentProxy* proxy, const int pageIndex);
        static void setSceneUuid(UBDocumentProxy* proxy, const int pageIndex, QUuid pUuid);

        static bool addElementToBeStored(QString domName,IDataStorage* dataStorageClass);

        static void convertPDFObjectsToImages(UBDocumentProxy* proxy);
        static void convertSvgImagesToImages(UBDocumentProxy* proxy);

        static QMap<QString,IDataStorage*> getAdditionalElementToStore() { return additionalElementToStore;}

        static const QString nsSvg;
        static const QString nsXLink;
        static const QString nsXHtml;
        static const QString nsUb;
        static const QString xmlTrue;
        static const QString xmlFalse;

        static const QString sFontSizePrefix;
        static const QString sPixelUnit;
        static const QString sFontWeightPrefix;
        static const QString sFontStylePrefix;

        static QDomDocument readTeacherGuideNode(int sceneIndex);
    private:

        static UBGraphicsScene* loadScene(UBDocumentProxy* proxy, const QByteArray& pArray);

        static QDomDocument loadSceneDocument(UBDocumentProxy* proxy, const int pPageIndex);

        static QString uniboardDocumentNamespaceUriFromVersion(int fileVersion);

        static const QString sFormerUniboardDocumentNamespaceUri;

        static QString toSvgTransform(const QMatrix& matrix);
        static QMatrix fromSvgTransform(const QString& transform);

        static QMap<QString,IDataStorage*> additionalElementToStore;

        static const QString SVG_STROKE_DOTLINE;




        class UBSvgSubsetReader
        {
            public:

                UBSvgSubsetReader(UBDocumentProxy* proxy, const QByteArray& pXmlData);

                virtual ~UBSvgSubsetReader(){}

                UBGraphicsScene* loadScene();

            private:

                UBGraphicsPolygonItem* polygonItemFromLineSvg(const QColor& pDefaultBrushColor);

                UBGraphicsPolygonItem* polygonItemFromPolygonSvg(const QColor& pDefaultBrushColor);

                QList<UBGraphicsPolygonItem*> polygonItemsFromPolylineSvg(const QColor& pDefaultColor);

                UBGraphicsPixmapItem* pixmapItemFromSvg();

                UBGraphicsSvgItem* svgItemFromSvg();

                UBGraphicsPDFItem* pdfItemFromPDF();

                UBGraphicsMediaItem* videoItemFromSvg();

                UBGraphicsMediaItem* audioItemFromSvg();

                UBGraphicsAppleWidgetItem* graphicsAppleWidgetFromSvg();

                UBGraphicsW3CWidgetItem* graphicsW3CWidgetFromSvg();

                UBGraphicsTextItem* textItemFromSvg();

                UBGraphicsCurtainItem* curtainItemFromSvg();

                UBGraphicsRuler* rulerFromSvg();

                UBGraphicsCompass* compassFromSvg();

                UBGraphicsProtractor* protractorFromSvg();

                UBGraphicsTriangle* triangleFromSvg();

                UBGraphicsCache* cacheFromSvg();

                QLinearGradient linearGradiantFromSvg();

                void getStyleFromSvg(UBAbstractGraphicsItem *item, const QColor &pDefaultPenColor);

                UB3HEditableGraphicsEllipseItem* shapeEllipseFromSvg(const QColor &pDefaultPenColor); // EV-7 - ALTI/AOU - 20131231

                UB1HEditableGraphicsCircleItem* shapeCircleFromSvg(const QColor &pDefaultPenColor);

                UB1HEditableGraphicsSquareItem* shapeSquareFromSvg(const QColor &pDefaultPenColor);

                UB3HEditableGraphicsRectItem* shapeRectFromSvg(const QColor &pDefaultPenColor); // EV-7 - ALTI/AOU - 20131231

                UBEditableGraphicsRegularShapeItem *shapeRegularFromSvg(const QColor& pDefaultPenColor);

                UBAbstractGraphicsPathItem* shapePathFromSvg(const QColor& pDefaultPenColor, int type); // EV-7 - ALTI/AOU - 20140102

                void readGroupRoot();
                QGraphicsItem *readElementFromGroup();
                UBGraphicsGroupContainerItem* readGroup(UBGraphicsItemAction *action = 0, QString uuid = "");

                void graphicsItemFromSvg(QGraphicsItem* gItem);

                qreal getZValueFromSvg();
                QUuid getUuidFromSvg();

                QXmlStreamReader mXmlReader;
                int mFileVersion;
                UBDocumentProxy *mProxy;
                QString mDocumentPath;

                QHash<QString,UBGraphicsStrokesGroup*> mStrokesList;

                QColor mGroupDarkBackgroundColor;
                QColor mGroupLightBackgroundColor;
                qreal mGroupZIndex;
                bool mGroupHasInfo;

                QString mNamespaceUri;
                UBGraphicsScene *mScene;
                UBGraphicsItemAction* readAction();
                QMap<QString,UBGraphicsStroke*> mStrokeList;
        };

        class UBSvgSubsetWriter
        {
            public:

                UBSvgSubsetWriter(UBDocumentProxy* proxy, UBGraphicsScene* pScene, const int pageIndex);

                bool persistScene(int pageIndex);

                virtual ~UBSvgSubsetWriter(){}

            private:

                void persistGroupToDom(QGraphicsItem *groupItem, QDomElement *curParent, QDomDocument *curDomDocument, UBGraphicsItemAction *action = 0);
                void persistStrokeToDom(QGraphicsItem *strokeItem, QDomElement *curParent, QDomDocument *curDomDocument);
                void polygonItemToSvgPolygon(UBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo, UBGraphicsItemAction* action = 0);
                void polygonItemToSvgLine(UBGraphicsPolygonItem* polygonItem, bool groupHoldsInfo);
                void strokeToSvgPolyline(UBGraphicsStroke* stroke, bool groupHoldsInfo);
                void strokeToSvgPolygon(UBGraphicsStroke* stroke, bool groupHoldsInfo);
                void writeAction(UBGraphicsItemAction* action);

                inline QString pointsToSvgPointsAttribute(QVector<QPointF> points)
                {
                    UBGeometryUtils::crashPointList(points);

                    int pointsCount = points.size();
                    QString svgPoints;

                    int length = 0;
                    QString sBuf;

                    for(int j = 0; j < pointsCount; j++)
                    {
                        sBuf = "%1,%2 ";
                        const QPointF & point = points.at(j);

                        QString temp1 =  "%1", temp2 = "%2";

                        temp1 = temp1.arg(point.x());
                        temp2 = temp2.arg(point.y());

                        QLocale loc(QLocale::C);
                        sBuf = sBuf.arg(loc.toFloat(temp1)).arg(loc.toFloat(temp2));

                        svgPoints.insert(length, sBuf);
                        length += sBuf.length();
                    }
                    return svgPoints;
                }

                inline qreal trickAlpha(qreal alpha)
                {
                        qreal trickAlpha = alpha;
                        if(trickAlpha >= 0.2 && trickAlpha < 0.6){
                                trickAlpha /= 5;
                        }else if (trickAlpha < 0.8)
                            trickAlpha /= 3;

                        return trickAlpha;
                }

                void pixmapItemToLinkedImage(UBGraphicsPixmapItem *pixmapItem, bool isBackground = false); // Issue 1684 - CFA 20131128
                void svgItemToLinkedSvg(UBGraphicsSvgItem *svgItem, bool isBackground = false);
                void pdfItemToLinkedPDF(UBGraphicsPDFItem *pdfItem);
                void videoItemToLinkedVideo(UBGraphicsMediaItem *videoItem);
                void audioItemToLinkedAudio(UBGraphicsMediaItem *audioItem);
                void graphicsItemToSvg(QGraphicsItem *item);
                void graphicsAppleWidgetToSvg(UBGraphicsAppleWidgetItem *item);
                void graphicsW3CWidgetToSvg(UBGraphicsW3CWidgetItem *item);
                void graphicsWidgetToSvg(UBGraphicsWidgetItem *item);
                void textItemToSvg(UBGraphicsTextItem *item);
                void curtainItemToSvg(UBGraphicsCurtainItem *item);
                void rulerToSvg(UBGraphicsRuler *item);
                void compassToSvg(UBGraphicsCompass *item);
                void protractorToSvg(UBGraphicsProtractor *item);
                void cacheToSvg(UBGraphicsCache* item);
                void triangleToSvg(UBGraphicsTriangle *item);

                void writeAbstractGraphicsItemGradient(UBAbstractGraphicsItem *item);
                void writeAbstractGraphicsItemStyle(UBAbstractGraphicsItem *item);

                void shapeEllipseToSvg(UB3HEditableGraphicsEllipseItem *item); // EV-7 - ALTI/AOU - 20131231
                void shapeRectToSvg(UB3HEditableGraphicsRectItem *item); // EV-7 - ALTI/AOU - 20131231
                void shapeSquareToSvg(UB1HEditableGraphicsSquareItem *item);
                void shapeCircleToSvg(UB1HEditableGraphicsCircleItem *item);
                void shapePathToSvg(UBAbstractGraphicsPathItem *item); // EV-7 - ALTI/AOU - 20131231
                void shapeRegularToSvg(UBEditableGraphicsRegularShapeItem *item);
                void writeSvgElement();

        private:

                UBGraphicsScene* mScene;
                QXmlStreamWriter mXmlWriter;
                QString mDocumentPath;
                int mPageIndex;
                UBDocumentProxy * mpDocument;   // Issue 1683 - ALTI/AOU - 20131212

        };
};

#endif /* UBSVGSUBSETADAPTOR_H_ */
