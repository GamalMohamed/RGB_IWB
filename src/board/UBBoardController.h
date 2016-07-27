#ifndef UBBOARDCONTROLLER_H_
#define UBBOARDCONTROLLER_H_

#include <QtGui>

#include <QObject>
#include "document/UBDocumentContainer.h"
#include "UBFeaturesController.h"
#include "gui/UBActionPalette.h"
#include "gui/UBToolbarButtonGroup.h"
#include "domain/UBShapeFactory.h"


class UBMainWindow;
class UBApplication;
class UBBoardView;

class UBDocumentController;
class UBMessageWindow;
class UBGraphicsScene;
class UBDocumentProxy;
class UBBlackoutWidget;
class UBToolWidget;
class UBVersion;
class UBSoftwareUpdate;
class UBSoftwareUpdateDialog;
class UBGraphicsMediaItem;
class UBGraphicsVideoItem;
class UBGraphicsAudioItem;
class UBGraphicsWidgetItem;
class UBBoardPaletteManager;
class UBItem;
class UBGraphicsItem;
class UBDocumentNavigator;


typedef enum{
    eItemActionType_Default,
    eItemActionType_Duplicate,
    eItemActionType_Paste
}eItemActionType;

class UBBoardController : public UBDocumentContainer
{
    Q_OBJECT

    public:
        UBBoardController(UBMainWindow *mainWindow);
        virtual ~UBBoardController();

        void init();
        void setupLayout();
        void setFinalPageSize();

        UBGraphicsScene* activeScene() const;
        int activeSceneIndex() const;
        QSize displayViewport();
        QSize controlViewport();
        QRectF controlGeometry();
        void closing();
        void addLinkToPage(QString sourceUrl, QSize size = QSize(340,200), QPointF pos = QPointF(0,0), const QString &embedCode = QString());

        int currentPage();

        QWidget* controlContainer()
        {
            return mControlContainer;
        }

        UBBoardView* controlView()
        {
            return mControlView;
        }

        UBBoardView* displayView()
        {
            return mDisplayView;
        }

        UBGraphicsScene* activeScene()
        {
            return mActiveScene;
        }

        void setPenColorOnDarkBackground(const QColor& pColor)
        {
            if (mPenColorOnDarkBackground == pColor)
                return;

            mPenColorOnDarkBackground = pColor;
            emit penColorChanged();
        }

        void setPenColorOnLightBackground(const QColor& pColor)
        {
            if (mPenColorOnLightBackground == pColor)
                return;

            mPenColorOnLightBackground = pColor;
            emit penColorChanged();
        }

        void setMarkerColorOnDarkBackground(const QColor& pColor)
        {
            mMarkerColorOnDarkBackground = pColor;
        }

        void setMarkerColorOnLightBackground(const QColor& pColor)
        {
            mMarkerColorOnLightBackground = pColor;
        }

        QColor penColorOnDarkBackground()
        {
            return mPenColorOnDarkBackground;
        }

        QColor penColorOnLightBackground()
        {
            return mPenColorOnLightBackground;
        }

        QColor markerColorOnDarkBackground()
        {
            return mMarkerColorOnDarkBackground;
        }

        QColor markerColorOnLightBackground()
        {
            return mMarkerColorOnLightBackground;
        }

        qreal systemScaleFactor()
        {
            return mSystemScaleFactor;
        }

        //EV-7 - NNE - 20140106
        UBShapeFactory& shapeFactory()
        {
            return mShapeFactory;
        }

        qreal currentZoom();

        void persistViewPositionOnCurrentScene();// Issue 1598/1605 - CFA - 20131028
        void persistCurrentScene(UBDocumentProxy *pProxy = 0);
        void showNewVersionAvailable(bool automatic, const UBVersion &installedVersion, const UBSoftwareUpdate &softwareUpdate);
        void setBoxing(QRect displayRect);
        void setToolbarTexts();
        static QUrl expandWidgetToTempDir(const QByteArray& pZipedData, const QString& pExtension = QString("wgt"));
//        static QRect freeRectInGlobalPos() const {return ;}
        void setPageSize(QSize newSize);
        UBBoardPaletteManager *paletteManager()
        {
            return mPaletteManager;
        }

        void notifyCache(bool visible);
        void notifyPageChanged();
        void displayMetaData(QMap<QString, QString> metadatas);

        void setActiveDocumentScene(UBDocumentProxy* pDocumentProxy, int pSceneIndex = 0, bool forceReload = false, const bool onImport = false);
        void setActiveDocumentScene(int pSceneIndex);

        void moveSceneToIndex(int source, int target);
        void duplicateScene(int index);
        UBGraphicsItem *duplicateItem(UBItem *item, bool bAsync = true, eItemActionType actionType = eItemActionType_Default);
        void deleteScene(int index);
        void regenerateThumbnails();

        bool cacheIsVisible() {return mCacheWidgetIsEnabled;}

        QString actionGroupText(){ return mActionGroupText;}
        QString actionUngroupText(){ return mActionUngroupText;}
        void setDocumentNavigator(UBDocumentNavigator *navigator){mDocumentNavigator = navigator;}
        UBDocumentNavigator *documentNavigator() const {return mDocumentNavigator;}

public slots:
        void ClearUndoStack();

        void showDocumentsDialog();
        void showKeyboard(bool show);
        void togglePodcast(bool checked);
        void blackout();
        void addScene();
        void addScene(UBDocumentProxy* proxy, int sceneIndex, bool replaceActiveIfEmpty = false);
        void addScene(UBGraphicsScene* scene, bool replaceActiveIfEmpty = false);
        void duplicateScene();
        void importPage();
        void clearScene();
        void clearSceneItems();
        void clearSceneAnnotation();

        // Issue 1684 - CFA - 20131120
        void setImageBackground(UBFeatureBackgroundDisposition disposition);
        void centerImageBackground();
        void adjustImageBackground();
        void mosaicImageBackground();
        void fillImageBackground();
        void extendImageBackground();

        void clearSceneBackground();
        void zoomIn(QPointF scenePoint = QPointF(0,0));
        void zoomOut(QPointF scenePoint = QPointF(0,0));
        void zoomRestore();
        void centerRestore();
        void centerOn(QPointF scenePoint = QPointF(0,0));
        void zoom(const qreal ratio, QPointF scenePoint);
        void handScroll(qreal dx, qreal dy);
        void previousScene();
        void nextScene();
        void firstScene();
        void lastScene();
        void groupButtonClicked();
        void downloadURL(const QUrl& url, QString contentSourceUrl = QString(), const QPointF& pPos = QPointF(0.0, 0.0), const QSize& pSize = QSize(), bool isBackground = false, bool internalData = false, UBFeatureBackgroundDisposition disposition = Center);
        UBItem *downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pHeader,
                                 QByteArray pData, QPointF pPos, QSize pSize,
                                 bool isSyncOperation = true, bool isBackground = false, bool internalData = false, eItemActionType actionType = eItemActionType_Default, UBFeatureBackgroundDisposition disposition = Center);
        void changeBackground(bool isDark, bool isCrossed);
        void setToolCursor(int tool);
        void showMessage(const QString& message, bool showSpinningWheel = false);
        void hideMessage();
        void setDisabled(bool disable);
        void setColorIndex(int pColorIndex);
        QColor inferOpposite(const QColor &candidate, const char tool);
        void removeTool(UBToolWidget* toolWidget);
        void hide();
        void show();
        void setWidePageSize(bool checked);
        void setWidePageSize16_10(bool checked);
        void setRegularPageSize(bool checked);
        void stylusToolChanged(int tool);
        void grabScene(const QRectF& pSceneRect);
        UBGraphicsMediaItem* addVideo(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        UBGraphicsMediaItem* addAudio(const QUrl& pUrl, bool startPlay, const QPointF& pos, bool bUseSource = false);
        UBGraphicsWidgetItem *addW3cWidget(const QUrl& pUrl, const QPointF& pos);

        void cut();
        void copy();
        void paste();
        void processMimeData(const QMimeData* pMimeData, const QPointF& pPos, eItemActionType actionType = eItemActionType_Default);
        void moveGraphicsWidgetToControlView(UBGraphicsWidgetItem* graphicWidget);
        void moveToolWidgetToScene(UBToolWidget* toolWidget);
        void addItem();

        void freezeW3CWidgets(bool freeze);
        void freezeW3CWidget(QGraphicsItem* item, bool freeze);
        void startScript();
        void stopScript();

        void ShowStylusDrawingOptions();
        void HideStylusDrawingOptions();

    signals:
        void newPageAdded();
        void activeSceneChanged();
        void zoomChanged(qreal pZoomFactor);
        void systemScaleFactorChanged(qreal pSystemScaleFactor);
        void penColorChanged();
        void controlViewportChanged();
        void backgroundChanged();
        void cacheEnabled();
        void cacheDisabled();
        void pageChanged();
        void documentReorganized(int index);
        void displayMetadata(QMap<QString, QString> metadata);
        void pageSelectionChanged(int index);
        void npapiWidgetCreated(const QString &Url);

    protected:
        void setupViews();
        void setupToolbar();
        void connectToolbar();
        void initToolbarTexts();
        void updateActionStates();
        void updateSystemScaleFactor();
        QString truncate(QString text, int maxWidth);

    protected slots:
        void selectionChanged();
        void undoRedoStateChange(bool canUndo);
        void documentSceneChanged(UBDocumentProxy* proxy, int pIndex);

    private:
        void updatePageSizeState();
        void saveViewState();
        void adjustDisplayViews();

        UBMainWindow *mMainWindow;
        UBGraphicsScene* mActiveScene;
        int mActiveSceneIndex;
        UBBoardPaletteManager *mPaletteManager;
        UBSoftwareUpdateDialog *mSoftwareUpdateDialog;
        UBMessageWindow *mMessageWindow;
        UBBoardView *mControlView;
        UBBoardView *mDisplayView;
        QWidget *mControlContainer;
        UBDocumentNavigator *mDocumentNavigator;
        QHBoxLayout *mControlLayout;
        qreal mZoomFactor;
        bool mIsClosing;
        QColor mPenColorOnDarkBackground;
        QColor mPenColorOnLightBackground;
        QColor mMarkerColorOnDarkBackground;
        QColor mMarkerColorOnLightBackground;
        qreal mSystemScaleFactor;
        bool mCleanupDone;
        QMap<QAction*, QPair<QString, QString> > mActionTexts;
        bool mCacheWidgetIsEnabled;
        QGraphicsItem* mLastCreatedItem;
        int mDeletingSceneIndex;
        int mMovingSceneIndex;
        QString mActionGroupText;
        QString mActionUngroupText;


        UBShapeFactory mShapeFactory;        


private slots:
        void stylusToolDoubleClicked(int tool);
        void boardViewResized(QResizeEvent* event);
        void documentWillBeDeleted(UBDocumentProxy* pProxy);
        void updateBackgroundActionsState(bool isDark, bool isCrossed);
        void updateBackgroundState();
        void colorPaletteChanged();
        void libraryDialogClosed(int ret);
        void lastWindowClosed();
        void onDownloadModalFinished();

};


#endif /* UBBOARDCONTROLLER_H_ */
