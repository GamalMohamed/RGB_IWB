#include "UBBoardPaletteManager.h"

#include "frameworks/UBPlatformUtils.h"
#include "frameworks/UBFileSystemUtils.h"

#include "core/UBApplication.h"
#include "core/UBApplicationController.h"
#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBDisplayManager.h"

#include "gui/UBMainWindow.h"
#include "gui/UBStylusPalette.h"
#include "gui/UBKeyboardPalette.h"
#include "gui/UBToolWidget.h"
#include "gui/UBZoomPalette.h"
#include "gui/UBWebToolsPalette.h"
#include "gui/UBActionPalette.h"
#include "gui/UBFavoriteToolPalette.h"
#include "gui/UBDockTeacherGuideWidget.h"
#include "gui/UBStartupHintsPalette.h"
#include "gui/UBCreateLinkPalette.h"

#include "web/UBWebPage.h"
#include "web/UBWebController.h"
#include "web/browser/WBBrowserWindow.h"
#include "web/browser/WBTabWidget.h"
#include "web/browser/WBWebView.h"

#include "desktop/UBDesktopAnnotationController.h"


#include "network/UBNetworkAccessManager.h"
#include "network/UBServerXMLHttpRequest.h"

#include "domain/UBGraphicsScene.h"
#include "domain/UBGraphicsPixmapItem.h"

#include "document/UBDocumentProxy.h"
#include "podcast/UBPodcastController.h"
#include "board/UBDrawingController.h"

#include "tools/UBToolsManager.h"

#include "UBBoardController.h"

#include "document/UBDocumentController.h"

#include "core/memcheck.h"

static bool hidden=false;
UBBoardPaletteManager::UBBoardPaletteManager(QWidget* container, UBBoardController* pBoardController)
    : QObject(container)
    , mKeyboardPalette(0)
    , mWebToolsCurrentPalette(0)
    , mContainer(container)
    , mBoardControler(pBoardController)
    , mStylusPalette(0)
    , mDrawingPalette(NULL)
    , mZoomPalette(0)
    , mTipPalette(0)
    , mLinkPalette(0)
    , mLeftPalette(NULL)
    , mRightPalette(NULL)
    , mBackgroundsPalette(0)
    , mToolsPalette(0)
    , mAddItemPalette(0)
    , mErasePalette(NULL)
    , mPagePalette(NULL)
    , mImageBackgroundPalette(NULL)
    , mEllipseActionPaletteButton(NULL)
    , mPendingPageButtonPressed(false)
    , mPendingZoomButtonPressed(false)
    , mPendingPanButtonPressed(false)
    , mPendingEraseButtonPressed(false)
    , mpPageNavigWidget(NULL)
    , mpCachePropWidget(NULL)
    , mpDownloadWidget(NULL)
    , mpTeacherGuideWidget(NULL)
    , mDownloadInProgress(false)
{
    mTeacherResources = NULL;
    connect(UBApplication::mainWindow->actionToolbox, SIGNAL(triggered()), this, SLOT(RightPaletteTools()));

    setupPalettes();
    connectPalettes();
}


UBBoardPaletteManager::~UBBoardPaletteManager()
{

// mAddedItemPalette is delete automatically because of is parent
// that changes depending on the mode

// mMainWindow->centralWidget is the parent of mStylusPalette
// do not delete this here.
}

void UBBoardPaletteManager::initPalettesPosAtStartup()
{
    mStylusPalette->initPosition();
    mDrawingPalette->initPosition();
}

void UBBoardPaletteManager::setupLayout()
{

}


void UBBoardPaletteManager::RightPaletteTools()
{
    mRightPalette->toggleCollapseExpand();
    if(!hidden)
    {
        toggleStylusPalette(false);
    }
    else
    {
        toggleStylusPalette(true);
    }
    hidden=!hidden;
}

/**
 * \brief Set up the dock palette widgets
 */
void UBBoardPaletteManager::setupDockPaletteWidgets()
{
    // Add the dock palettes
    mLeftPalette = new UBLeftPalette(mContainer);

    // LEFT palette widgets
    mpPageNavigWidget = new UBPageNavigationWidget();
    mLeftPalette->registerWidget(mpPageNavigWidget);
    mLeftPalette->addTab(mpPageNavigWidget);

    if(UBSettings::settings()->teacherGuidePageZeroActivated->get().toBool() ||
            UBSettings::settings()->teacherGuideLessonPagesActivated->get().toBool())
    {
        mpTeacherGuideWidget = new UBDockTeacherGuideWidget();
        mLeftPalette->registerWidget(mpTeacherGuideWidget);
        mLeftPalette->addTab(mpTeacherGuideWidget);
    }

    mTeacherResources = new UBDockResourcesWidget;
    mLeftPalette->registerWidget(mTeacherResources);
    mLeftPalette->addTab(mTeacherResources);

    mLeftPalette->connectSignals();


    // Create the widgets for the dock palettes
    mpCachePropWidget = new UBCachePropertiesWidget();
    mpDownloadWidget = new UBDockDownloadWidget();


    mRightPalette = new UBRightPalette(mContainer);

    // RIGHT palette widgets
    mpFeaturesWidget = new UBFeaturesWidget();
    mRightPalette->registerWidget(mpFeaturesWidget);
    mRightPalette->addTab(mpFeaturesWidget);

    // The cache widget will be visible only if a cache is put on the page
    mRightPalette->registerWidget(mpCachePropWidget);

    //  The download widget will be part of the right palette but
    //  will become visible only when the first download starts
    mRightPalette->registerWidget(mpDownloadWidget);
    mRightPalette->connectSignals();
    changeMode(eUBDockPaletteWidget_BOARD, true);

    // Hide the tabs that must be hidden
    mRightPalette->removeTab(mpDownloadWidget);
    mRightPalette->removeTab(mpCachePropWidget);
    mRightPalette->Hide();

}

void UBBoardPaletteManager::setupPalettes()
{

    if (UBPlatformUtils::hasVirtualKeyboard())
    {
        mKeyboardPalette = new UBKeyboardPalette(0);
        #ifndef Q_WS_WIN
                connect(mKeyboardPalette, SIGNAL(closed()), mKeyboardPalette, SLOT(onDeactivated()));
        #endif
    }


    setupDockPaletteWidgets();

    // Add the other palettes
    mStylusPalette = new UBStylusPalette(mContainer, UBSettings::settings()->appToolBarOrientationVertical->get().toBool() ? Qt::Vertical : Qt::Horizontal);
    connect(mStylusPalette, SIGNAL(stylusToolDoubleClicked(int)), UBApplication::boardController, SLOT(stylusToolDoubleClicked(int)));
    mStylusPalette->show(); // Show stylus palette at startup

    mDrawingPalette = new UBDrawingPalette(mContainer, UBSettings::settings()->appDrawingPaletteOrientationHorizontal->get().toBool() ? Qt::Horizontal : Qt::Vertical);
    mDrawingPalette->hide();

    mZoomPalette = new UBZoomPalette(mContainer);
    mStylusPalette->stackUnder(mZoomPalette);
    mDrawingPalette->stackUnder(mZoomPalette);

    mTipPalette = new UBStartupHintsPalette(mContainer);


    QList<QAction*> backgroundsActions;
    backgroundsActions << UBApplication::mainWindow->actionPlainLightBackground;
    backgroundsActions << UBApplication::mainWindow->actionCrossedLightBackground;
    backgroundsActions << UBApplication::mainWindow->actionPlainDarkBackground;
    backgroundsActions << UBApplication::mainWindow->actionCrossedDarkBackground;

    mBackgroundsPalette = new UBActionPalette(backgroundsActions, Qt::Horizontal , mContainer);
    mBackgroundsPalette->setButtonIconSize(QSize(128, 128));
    mBackgroundsPalette->groupActions();
    mBackgroundsPalette->setClosable(true);
    mBackgroundsPalette->setAutoClose(true);
    mBackgroundsPalette->adjustSizeAndPosition();
    mBackgroundsPalette->hide();


    QList<QAction*> addItemActions;
    addItemActions << UBApplication::mainWindow->actionAddItemToCurrentPage;
    addItemActions << UBApplication::mainWindow->actionAddItemToNewPage;
    addItemActions << UBApplication::mainWindow->actionAddItemToLibrary;

    mAddItemPalette = new UBActionPalette(addItemActions, Qt::Horizontal, mContainer);
    mAddItemPalette->setButtonIconSize(QSize(128, 128));
    mAddItemPalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mAddItemPalette->groupActions();
    mAddItemPalette->setClosable(true);
    mAddItemPalette->adjustSizeAndPosition();
    mAddItemPalette->hide();


    QList<QAction*> eraseActions;
    eraseActions << UBApplication::mainWindow->actionEraseAnnotations;
    eraseActions << UBApplication::mainWindow->actionEraseItems;
    eraseActions << UBApplication::mainWindow->actionClearPage;
    eraseActions << UBApplication::mainWindow->actionEraseBackground;

    mErasePalette = new UBActionPalette(eraseActions, Qt::Horizontal , mContainer);
    mErasePalette->setButtonIconSize(QSize(128, 128));
    mErasePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mErasePalette->groupActions();
    mErasePalette->setClosable(true);
    mErasePalette->adjustSizeAndPosition();
    mErasePalette->hide();


    QList<QAction*> pageActions;
    pageActions << UBApplication::mainWindow->actionNewPage;
    pageActions << UBApplication::mainWindow->actionDuplicatePage;
    pageActions << UBApplication::mainWindow->actionImportPage;

    mPagePalette = new UBActionPalette(pageActions, Qt::Horizontal , mContainer);
    mPagePalette->setButtonIconSize(QSize(128, 128));
    mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mPagePalette->groupActions();
    mPagePalette->setClosable(true);
    mPagePalette->adjustSizeAndPosition();
    mPagePalette->hide();


    QList<QAction*> imageBackgroundActions;
    imageBackgroundActions << UBApplication::mainWindow->actionCenterImageBackground;
    UBApplication::mainWindow->actionCenterImageBackground->setIcon(QIcon(":/images/imageBackgroundPalette/centerBackground.png"));
    imageBackgroundActions << UBApplication::mainWindow->actionAdjustImageBackground;
    UBApplication::mainWindow->actionAdjustImageBackground->setIcon(QIcon(":/images/imageBackgroundPalette/adjustBackground.png"));
    imageBackgroundActions << UBApplication::mainWindow->actionMosaicImageBackground;
    UBApplication::mainWindow->actionMosaicImageBackground->setIcon(QIcon(":/images/imageBackgroundPalette/mosaicBackground.png"));
    imageBackgroundActions << UBApplication::mainWindow->actionFillImageBackground;
    UBApplication::mainWindow->actionFillImageBackground->setIcon(QIcon(":/images/imageBackgroundPalette/fillBackground.png"));
    imageBackgroundActions << UBApplication::mainWindow->actionExtendImageBackground;
    UBApplication::mainWindow->actionExtendImageBackground->setIcon(QIcon(":/images/imageBackgroundPalette/extendBackground.png"));

    mImageBackgroundPalette = new UBActionPalette(imageBackgroundActions, Qt::Horizontal , mContainer);
    mImageBackgroundPalette->setButtonIconSize(QSize(128, 128));
    mImageBackgroundPalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mImageBackgroundPalette->groupActions();
    mImageBackgroundPalette->setClosable(true);
    mImageBackgroundPalette->adjustSizeAndPosition();
    mImageBackgroundPalette->hide();


    connect(UBSettings::settings()->appToolBarOrientationVertical, SIGNAL(changed(QVariant)), this, SLOT(changeStylusPaletteOrientation(QVariant)));
}

void UBBoardPaletteManager::connectPalettes()
{
    connect(UBApplication::mainWindow->actionDrawing, SIGNAL(toggled(bool)), this, SLOT(toggleDrawingPalette(bool)));
    connect(UBApplication::mainWindow->actionStylus, SIGNAL(toggled(bool)), this, SLOT(toggleStylusPalette(bool)));

    foreach(QWidget *widget, UBApplication::mainWindow->actionZoomIn->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(zoomButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(zoomButtonReleased()));
        }
    }

    foreach(QWidget *widget, UBApplication::mainWindow->actionZoomOut->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(zoomButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(zoomButtonReleased()));
        }
    }

    foreach(QWidget *widget, UBApplication::mainWindow->actionHand->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(panButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(panButtonReleased()));
        }
    }

    connect(UBApplication::mainWindow->actionBackgrounds, SIGNAL(toggled(bool)), this, SLOT(toggleBackgroundPalette(bool)));
    connect(mBackgroundsPalette, SIGNAL(closed()), this, SLOT(backgroundPaletteClosed()));

    connect(UBApplication::mainWindow->actionPlainLightBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(UBApplication::mainWindow->actionCrossedLightBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(UBApplication::mainWindow->actionPlainDarkBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(UBApplication::mainWindow->actionCrossedDarkBackground, SIGNAL(triggered()), this, SLOT(changeBackground()));
    connect(UBApplication::mainWindow->actionPodcast, SIGNAL(triggered(bool)), this, SLOT(tooglePodcastPalette(bool)));

    connect(UBApplication::mainWindow->actionAddItemToCurrentPage, SIGNAL(triggered()), this, SLOT(addItemToCurrentPage()));
    connect(UBApplication::mainWindow->actionAddItemToNewPage, SIGNAL(triggered()), this, SLOT(addItemToNewPage()));
    connect(UBApplication::mainWindow->actionAddItemToLibrary, SIGNAL(triggered()), this, SLOT(addItemToLibrary()));

    // Issue 1684 - CFA - 20131119
    connect(UBApplication::mainWindow->actionEraseItems, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionEraseAnnotations, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionClearPage, SIGNAL(triggered()), mErasePalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionEraseBackground,SIGNAL(triggered()),mErasePalette,SLOT(close()));
    connect(mErasePalette, SIGNAL(closed()), this, SLOT(erasePaletteClosed()));

    connect(UBApplication::mainWindow->actionCenterImageBackground, SIGNAL(triggered()), mImageBackgroundPalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionAdjustImageBackground, SIGNAL(triggered()),mImageBackgroundPalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionExtendImageBackground, SIGNAL(triggered()), mImageBackgroundPalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionFillImageBackground,SIGNAL(triggered()),mImageBackgroundPalette,SLOT(close()));
    connect(UBApplication::mainWindow->actionMosaicImageBackground,SIGNAL(triggered()),mImageBackgroundPalette,SLOT(close()));

    foreach(QWidget *widget, UBApplication::mainWindow->actionErase->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(erasePaletteButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(erasePaletteButtonReleased()));
        }
    }

    connect(UBApplication::mainWindow->actionNewPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionDuplicatePage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(UBApplication::mainWindow->actionImportPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
    connect(mPagePalette, SIGNAL(closed()), this, SLOT(pagePaletteClosed()));

    foreach(QWidget *widget, UBApplication::mainWindow->actionPages->associatedWidgets())
    {
        QAbstractButton *button = qobject_cast<QAbstractButton*>(widget);
        if (button)
        {
            connect(button, SIGNAL(pressed()), this, SLOT(pagePaletteButtonPressed()));
            connect(button, SIGNAL(released()), this, SLOT(pagePaletteButtonReleased()));
        }
    }
}


void UBBoardPaletteManager::slot_changeMainMode(UBApplicationController::MainMode mainMode)
{
//    Board = 0, Internet, Document, Tutorial, ParaschoolEditor, WebDocument

    switch( mainMode )
    {
        case UBApplicationController::Board:
            {
                // call changeMode only when switch NOT from desktop mode
                if(!UBApplication::applicationController->isShowingDesktop())
                    changeMode(eUBDockPaletteWidget_BOARD);
            }
            break;

        case UBApplicationController::Tutorial:
            {
                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                    mKeyboardPalette->hide();
            }
            break;

        case UBApplicationController::Internet:
            changeMode(eUBDockPaletteWidget_WEB);
            break;

        case UBApplicationController::Document:
            changeMode(eUBDockPaletteWidget_DOCUMENT);
            break;

        default:
            {
                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                    mKeyboardPalette->hide();
            }
            break;
    }
}

void UBBoardPaletteManager::slot_changeDesktopMode(bool isDesktop)
{
    UBApplicationController::MainMode currMode = UBApplication::applicationController->displayMode();
    if(!isDesktop)
    {
        switch( currMode )
        {
            case UBApplicationController::Board:
                changeMode(eUBDockPaletteWidget_BOARD);
                break;

            default:
                break;
        }
    }
    else
        changeMode(eUBDockPaletteWidget_DESKTOP);
}


void UBBoardPaletteManager::pagePaletteButtonPressed()
{
    mPageButtonPressedTime = QTime::currentTime();

    mPendingPageButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(pagePaletteButtonReleased()));
}

void UBBoardPaletteManager::pagePaletteButtonReleased()
{
    if (mPendingPageButtonPressed)
    {
        if( mPageButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            // The palette is reinstanciated because the duplication depends on the current scene
            delete(mPagePalette);
            mPagePalette = 0;
            QList<QAction*>pageActions;
            pageActions << UBApplication::mainWindow->actionNewPage;
            UBBoardController* boardController = UBApplication::boardController;
            if(UBApplication::documentController->pageCanBeDuplicated(UBDocumentContainer::pageFromSceneIndex(boardController->activeSceneIndex()))){
                pageActions << UBApplication::mainWindow->actionDuplicatePage;
            }
            pageActions << UBApplication::mainWindow->actionImportPage;

            mPagePalette = new UBActionPalette(pageActions, Qt::Horizontal , mContainer);
            mPagePalette->setButtonIconSize(QSize(128, 128));
            mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            mPagePalette->groupActions();
            mPagePalette->setClosable(true);

            // As we recreate the pagePalette every time, we must reconnect the slots
            connect(UBApplication::mainWindow->actionNewPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(UBApplication::mainWindow->actionDuplicatePage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(UBApplication::mainWindow->actionImportPage, SIGNAL(triggered()), mPagePalette, SLOT(close()));
            connect(mPagePalette, SIGNAL(closed()), this, SLOT(pagePaletteClosed()));

            togglePagePalette(true);
        }
        else
        {
            UBApplication::mainWindow->actionNewPage->trigger();
        }

        mPendingPageButtonPressed = false;
    }
}

void UBBoardPaletteManager::erasePaletteButtonPressed()
{
    mEraseButtonPressedTime = QTime::currentTime();

    mPendingEraseButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(erasePaletteButtonReleased()));
}

void UBBoardPaletteManager::erasePaletteButtonReleased()
{
    if (mPendingEraseButtonPressed)
    {
        if( mEraseButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            toggleErasePalette(true);
        }
        else
        {
            UBApplication::mainWindow->actionClearPage->trigger();
        }

        mPendingEraseButtonPressed = false;
    }
}


void UBBoardPaletteManager::linkClicked(const QUrl& url)
{
      UBApplication::applicationController->showInternet();
      UBApplication::webController->loadUrl(url);
}

void UBBoardPaletteManager::purchaseLinkActivated(const QString& link)
{
    UBApplication::applicationController->showInternet();
    UBApplication::webController->loadUrl(QUrl(link));
}


bool isFirstResized = true;
void UBBoardPaletteManager::containerResized()
{
    int innerMargin = UBSettings::boardMargin;

    int userLeft = innerMargin;
    int userWidth = mContainer->width() - (2 * innerMargin);
    int userTop = innerMargin;
    int userHeight = mContainer->height() - (2 * innerMargin);

    if(mStylusPalette)
    {
        //mStylusPalette->move(userLeft, userTop);
        mStylusPalette->adjustSizeAndPosition(true,false);
        mStylusPalette->initPosition();
    }

    if (mDrawingPalette)
    {
        mDrawingPalette->adjustSizeAndPosition(true,false);
        mDrawingPalette->initPosition();
    }

    if(mZoomPalette)
    {
        mZoomPalette->move(userLeft + userWidth - mZoomPalette->width()
                , userTop + userHeight /*- mPageNumberPalette->height()*/ - innerMargin - mZoomPalette->height());
        mZoomPalette->adjustSizeAndPosition(true,false);
    }

    if (isFirstResized && mKeyboardPalette && mKeyboardPalette->parent() == UBApplication::boardController->controlContainer())
    {
        isFirstResized = false;
        mKeyboardPalette->move(userLeft + (userWidth - mKeyboardPalette->width())/2,
                               userTop + (userHeight - mKeyboardPalette->height())/2);
        mKeyboardPalette->adjustSizeAndPosition();
    }

    if(mLeftPalette)
        mLeftPalette->resize(mLeftPalette->width()-1, mContainer->height());
}

void UBBoardPaletteManager::changeBackground()
{
    if (UBApplication::mainWindow->actionCrossedLightBackground->isChecked())
        UBApplication::boardController->changeBackground(false, true);
    else if (UBApplication::mainWindow->actionPlainDarkBackground->isChecked())
        UBApplication::boardController->changeBackground(true, false);
    else if (UBApplication::mainWindow->actionCrossedDarkBackground->isChecked())
        UBApplication::boardController->changeBackground(true, true);
    else
        UBApplication::boardController->changeBackground(false, false);

    UBApplication::mainWindow->actionBackgrounds->setChecked(false);
}

void UBBoardPaletteManager::activeSceneChanged()
{
    UBGraphicsScene *activeScene =  UBApplication::boardController->activeScene();
    int pageIndex = UBApplication::boardController->activeSceneIndex();

    if (mStylusPalette)
        connect(mStylusPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideEraser()));

    if (mpPageNavigWidget)
    {
        mpPageNavigWidget->setPageNumber(UBDocumentContainer::pageFromSceneIndex(pageIndex), activeScene->document()->pageCount());
    }

    //issue 1682 - NNE - 20140113
    if(pageIndex > 0){
        int currentTabIndex = mLeftPalette->currentTabIndex();
        mLeftPalette->onShowTabWidget(mTeacherResources); // ALTI/AOU - 20140217 : instead of addTab(), we use onShowTabWidget() because it calls moveTabs().
        mLeftPalette->showTabWidget(currentTabIndex); // Stay on same tab. Don't go to the added tab.
    }else{
        mLeftPalette->onHideTabWidget(mTeacherResources); // ALTI/AOU - 20140217 : instead of removeTab(), we use onHideTabWidget() because it calls moveTabs().
        mLeftPalette->showTabWidget(mLeftPalette->currentTabIndex());
    }
    //issue 1682 - NNE - 20140113 : END

    if (mZoomPalette)
        connect(mZoomPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideEraser()));

    if (mBackgroundsPalette)
        connect(mBackgroundsPalette, SIGNAL(mouseEntered()), activeScene, SLOT(hideEraser()));
}

void UBBoardPaletteManager::toggleBackgroundPalette(bool checked)
{
    mBackgroundsPalette->setVisible(checked);

    if (checked)
    {
        UBApplication::mainWindow->actionErase->setChecked(false);
        UBApplication::mainWindow->actionNewPage->setChecked(false);

        mBackgroundsPalette->adjustSizeAndPosition();
    }
}

void UBBoardPaletteManager::backgroundPaletteClosed()
{
    UBApplication::mainWindow->actionBackgrounds->setChecked(false);
}

void UBBoardPaletteManager::toggleStylusPalette(bool checked)
{
    mStylusPalette->setVisible(checked);
}

void UBBoardPaletteManager::toggleDrawingPalette(bool checked)
{
    mDrawingPalette->setVisible(checked);
}

void UBBoardPaletteManager::toggleErasePalette(bool checked)
{
    mErasePalette->setVisible(checked);
    if (checked)
    {
        UBApplication::mainWindow->actionBackgrounds->setChecked(false);
        UBApplication::mainWindow->actionNewPage->setChecked(false);

        mErasePalette->adjustSizeAndPosition();
    }
}

void UBBoardPaletteManager::erasePaletteClosed()
{
    UBApplication::mainWindow->actionErase->setChecked(false);
}

void UBBoardPaletteManager::toggleImageBackgroundPalette(bool checked, bool isDefault)
{
    mImageBackgroundPalette->setVisible(checked);
    UBApplication::boardController->selectedDocument()->setHasDefaultImageBackground(isDefault);
    if (checked)
    {
        UBApplication::mainWindow->actionBackgrounds->setChecked(false);
        UBApplication::mainWindow->actionErase->setChecked(false);

        mImageBackgroundPalette->adjustSizeAndPosition();
    }
}

void UBBoardPaletteManager::togglePagePalette(bool checked)
{
    mPagePalette->setVisible(checked);
    if (checked)
    {
        UBApplication::mainWindow->actionBackgrounds->setChecked(false);
        UBApplication::mainWindow->actionErase->setChecked(false);

        mPagePalette->adjustSizeAndPosition();
    }
}

void UBBoardPaletteManager::pagePaletteClosed()
{
    UBApplication::mainWindow->actionPages->setChecked(false);
}

void UBBoardPaletteManager::tooglePodcastPalette(bool checked)
{
    UBPodcastController::instance()->toggleRecordingPalette(checked);
}


void UBBoardPaletteManager::addItem(const QUrl& pUrl)
{
    mItemUrl = pUrl;
    mPixmap = QPixmap();
    mPos = QPointF(0, 0);
    mScaleFactor = 1.;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();
}

void UBBoardPaletteManager::changeMode(eUBDockPaletteWidgetMode newMode, bool isInit)
{
    bool rightPaletteVisible = mRightPalette->switchMode(newMode);
    bool leftPaletteVisible = mLeftPalette->switchMode(newMode);

    if (newMode != eUBDockPaletteWidget_BOARD)
    {
        if (mBackgroundsPalette)
            mBackgroundsPalette->savePos();
        if (mKeyboardPalette)
            mKeyboardPalette->savePos();
        if (mZoomPalette)
            mZoomPalette->savePos();
        if (mPagePalette)
            mPagePalette->savePos();
        if (mErasePalette)
            mErasePalette->savePos();
        if (mAddItemPalette)
            mAddItemPalette->savePos();
    }
    else
    {
        if (mBackgroundsPalette)
            mBackgroundsPalette->restorePos();
        if (mKeyboardPalette)
            mKeyboardPalette->restorePos();
        if (mZoomPalette)
            mZoomPalette->restorePos();
        if (mPagePalette)
            mPagePalette->restorePos();
        if (mErasePalette)
            mErasePalette->restorePos();
        if (mAddItemPalette)
            mAddItemPalette->restorePos();
    }


    switch( newMode )
    {
        case eUBDockPaletteWidget_BOARD:
            {
                // On Application start up the mAddItemPalette isn't initialized yet
                if(mAddItemPalette){
                    mAddItemPalette->setParent(UBApplication::boardController->controlContainer());
                }
                mLeftPalette->assignParent(mContainer);
                mRightPalette->assignParent(mContainer);

                if (mDrawingPalette)
                    mDrawingPalette->stackUnder(mStylusPalette);

                mRightPalette->stackUnder(mDrawingPalette);
                mLeftPalette->stackUnder(mDrawingPalette);

                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(UBApplication::boardController->controlContainer());
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(UBApplication::boardController->controlContainer());
                }

                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_WS_WIN
                if (rightPaletteVisible)
                    mRightPalette->setAdditionalVOffset(0);
#endif

                if( !isInit )
                    containerResized();
                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        case eUBDockPaletteWidget_DESKTOP:
            {
                mAddItemPalette->setParent((QWidget*)UBApplication::applicationController->uninotesController()->drawingView());
                mLeftPalette->assignParent((QWidget*)UBApplication::applicationController->uninotesController()->drawingView());
                mRightPalette->assignParent((QWidget*)UBApplication::applicationController->uninotesController()->drawingView());
                mStylusPalette->raise();
                mDrawingPalette->raise();

                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
#ifndef Q_WS_X11
                        mKeyboardPalette->setParent((QWidget*)UBApplication::applicationController->uninotesController()->drawingView());
#else
                        mKeyboardPalette->setParent(0);
#endif
#ifdef Q_WS_MAC
                        mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif
                        mKeyboardPalette->show();
                    }
                    else
// In linux keyboard in desktop mode have to allways be with null parent
#ifdef Q_WS_X11
                        mKeyboardPalette->setParent(0);
#else
                        mKeyboardPalette->setParent((QWidget*)UBApplication::applicationController->uninotesController()->drawingView());
#endif //Q_WS_X11
#ifdef Q_WS_MAC
                        mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif

                }

                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_WS_WIN
                if (rightPaletteVisible)
                {
                    if (UBSettings::settings()->appToolBarPositionedAtTop->get().toBool())
                        mRightPalette->setAdditionalVOffset(30);
                    else
                    {
                        QDesktopWidget *desktop = QApplication::desktop();
                        int taskBarOffset = desktop->screenGeometry(mRightPalette).height() - desktop->availableGeometry(mRightPalette).height();
                        mRightPalette->setAdditionalVOffset(-taskBarOffset);
                    }
                }
#endif

                if(!isInit)
                    UBApplication::applicationController->uninotesController()->TransparentWidgetResized();

                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        case eUBDockPaletteWidget_WEB:
            {
                mAddItemPalette->setParent(UBApplication::mainWindow);

                mRightPalette->assignParent(UBApplication::webController->GetCurrentWebBrowser());
                mRightPalette->setVisible(rightPaletteVisible);

                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                {
                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(UBApplication::mainWindow);
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(UBApplication::mainWindow);
                }

            }
            break;

        case eUBDockPaletteWidget_DOCUMENT:
            {
                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
                mLeftPalette->assignParent(UBApplication::documentController->controlView());
                mRightPalette->assignParent(UBApplication::documentController->controlView());
                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(UBApplication::documentController->controlView());
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(UBApplication::documentController->controlView());
                }
                if (mWebToolsCurrentPalette)
                    mWebToolsCurrentPalette->hide();
            }
            break;

        default:
            {
                mLeftPalette->setVisible(leftPaletteVisible);
                mRightPalette->setVisible(rightPaletteVisible);
                mLeftPalette->assignParent(0);
                mRightPalette->assignParent(0);
                if (UBPlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                {

                    if(mKeyboardPalette->m_isVisible)
                    {
                        mKeyboardPalette->hide();
                        mKeyboardPalette->setParent(0);
                        mKeyboardPalette->show();
                    }
                    else
                        mKeyboardPalette->setParent(0);
                }
            }
            break;
    }

    if( !isInit )
        UBApplication::boardController->notifyPageChanged();
}

void UBBoardPaletteManager::addItem(const QPixmap& pPixmap, const QPointF& pos,  qreal scaleFactor, const QUrl& sourceUrl)
{
    mItemUrl = sourceUrl;
    mPixmap = pPixmap;
    mPos = pos;
    mScaleFactor = scaleFactor;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();
}

void UBBoardPaletteManager::addItemToCurrentPage()
{
    //Issue NC - CFA - 20140331 : retour au mode board si mode desktop (en mode desktop, displayMode() renvoie Board...)
    if (UBApplication::applicationController->displayMode() != UBApplicationController::Board || UBApplication::applicationController->isShowingDesktop())
        UBApplication::applicationController->showBoard();

    mAddItemPalette->hide();
    if(mPixmap.isNull())
        UBApplication::boardController->downloadURL(mItemUrl);
    else
    {
        UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);

        UBGraphicsPixmapItem* item = UBApplication::boardController->activeScene()->addPixmap(mPixmap, NULL, mPos, mScaleFactor);

        item->setSourceUrl(mItemUrl);
        item->setSelected(true);       
    }
}

void UBBoardPaletteManager::addItemToNewPage()
{
    UBApplication::boardController->addScene();
    addItemToCurrentPage();
}

void UBBoardPaletteManager::addItemToLibrary()
{
    if(mPixmap.isNull())
    {
       mPixmap = QPixmap(mItemUrl.toLocalFile());
    }

    if(!mPixmap.isNull())
    {
        if(mScaleFactor != 1.)
        {
             mPixmap = mPixmap.scaled(mScaleFactor * mPixmap.width(), mScaleFactor* mPixmap.height()
                     , Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        QImage image = mPixmap.toImage();

        QDateTime now = QDateTime::currentDateTime();
        QString capturedName  = tr("CapturedImage") + "-" + now.toString("dd-MM-yyyy hh-mm-ss") + ".png";
        mpFeaturesWidget->importImage(image, capturedName);
    }
    else
    {
        UBApplication::showMessage(tr("Error Adding Image to Library"));
    }

    mAddItemPalette->hide();
}


void UBBoardPaletteManager::zoomButtonPressed()
{
    mZoomButtonPressedTime = QTime::currentTime();

    mPendingZoomButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(zoomButtonReleased()));
}

void UBBoardPaletteManager::zoomButtonReleased()
{
    if (mPendingZoomButtonPressed)
    {
        if(mZoomButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            mBoardControler->zoomRestore();
        }

        mPendingZoomButtonPressed = false;
    }
}

void UBBoardPaletteManager::panButtonPressed()
{
    mPanButtonPressedTime = QTime::currentTime();

    mPendingPanButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(panButtonReleased()));
}


void UBBoardPaletteManager::panButtonReleased()
{
    if (mPendingPanButtonPressed)
    {
        if(mPanButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            mBoardControler->centerRestore();
        }

        mPendingPanButtonPressed = false;
    }
}

void UBBoardPaletteManager::showVirtualKeyboard(bool show)
{
    if (mKeyboardPalette)
        mKeyboardPalette->setVisible(show);
}

void UBBoardPaletteManager::changeStylusPaletteOrientation(QVariant var)
{
    bool bVertical = var.toBool();
    bool bVisible = mStylusPalette->isVisible();

    // Clean the old palette
    if(NULL != mStylusPalette)
    {
        delete mStylusPalette;
        mStylusPalette = NULL;
    }

    // Create the new palette
    if(bVertical)
    {
        mStylusPalette = new UBStylusPalette(mContainer, Qt::Vertical);
    }
    else
    {
        mStylusPalette = new UBStylusPalette(mContainer, Qt::Horizontal);
    }

    connect(mStylusPalette, SIGNAL(stylusToolDoubleClicked(int)), UBApplication::boardController, SLOT(stylusToolDoubleClicked(int)));
    mStylusPalette->setVisible(bVisible); // always show stylus palette at startup
    mDrawingPalette->initPosition(); // move de drawing Palette
}


void UBBoardPaletteManager::connectToDocumentController()
{
    emit connectToDocController();
}

void UBBoardPaletteManager::refreshPalettes()
{
    mRightPalette->update();
    mLeftPalette->update();
}

void UBBoardPaletteManager::startDownloads()
{
    if(!mDownloadInProgress)
    {
        mDownloadInProgress = true;
        mpDownloadWidget->setVisibleState(true);
        mRightPalette->addTab(mpDownloadWidget);
    }
}

void UBBoardPaletteManager::stopDownloads()
{
    if(mDownloadInProgress)
    {
        mDownloadInProgress = false;
        mpDownloadWidget->setVisibleState(false);
        mRightPalette->removeTab(mpDownloadWidget);
    }
}


UBCreateLinkPalette* UBBoardPaletteManager::linkPalette()
{
    if(mLinkPalette)
        delete mLinkPalette;
    mLinkPalette = new UBCreateLinkPalette(mContainer);
    return mLinkPalette;
}

