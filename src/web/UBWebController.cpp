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



#include <QtGui>
#include <QDomDocument>
#include <QXmlQuery>

#include "frameworks/UBPlatformUtils.h"

#include "UBWebController.h"
#include "UBOEmbedParser.h"
#include "UBTrapFlashController.h"

#include "web/browser/WBBrowserWindow.h"
#include "web/browser/WBWebView.h"
#include "web/browser/WBDownloadManager.h"
#include "web/browser/WBTabWidget.h"

#include "network/UBServerXMLHttpRequest.h"
#include "network/UBNetworkAccessManager.h"

#include "gui/UBWidgetMirror.h"
#include "gui/UBMainWindow.h"
#include "gui/UBWebToolsPalette.h"
#include "gui/UBKeyboardPalette.h"

#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBApplication.h"
#include "core/UBApplicationController.h"
#include "core/UBDisplayManager.h"

#include "board/UBBoardController.h"
#include "board/UBDrawingController.h"

#include "domain/UBGraphicsScene.h"

#include "desktop/UBCustomCaptureWindow.h"
#include "board/UBBoardPaletteManager.h"

#include "core/memcheck.h"

UBWebController::UBWebController(UBMainWindow* mainWindow)
    : QObject(mainWindow->centralWidget())
    , mMainWindow(mainWindow)
    , mCurrentWebBrowser(0)
    , mBrowserWidget(0)
    , mTrapContentController(0)
    , mToolsCurrentPalette(0)
    , mToolsPalettePositionned(false)
    , mDownloadViewIsVisible(false)
{
    connect(mMainWindow->actionWebTools, SIGNAL(toggled(bool)), this, SLOT(toggleWebToolsPalette(bool)));
    connect(mMainWindow->actionBookmark, SIGNAL(triggered()), this, SLOT(onActionBookmark()));
    mStackedWidget = new QStackedWidget();
    mStackedWidget->addWidget(new QWidget(mStackedWidget));
    mStackedWidget->addWidget(new QWidget(mStackedWidget));
    mStackedWidget->addWidget(new QWidget(mStackedWidget));

    mMainWindow->addWebWidget(mStackedWidget);

    for (int i = 0; i < TotalNumberOfWebInstances; i += 1){
        mWebBrowserList[i] = 0;
        mToolsPaletteList[i] = 0;
        mToolsPalettePositionnedList[i] = false;
    }

    connect(&mOEmbedParser, SIGNAL(oembedParsed(QVector<sOEmbedContent>)), this, SLOT(onOEmbedParsed(QVector<sOEmbedContent>)));

    // TODO : Comment the next line to continue the Youtube button bugfix
    initialiazemOEmbedProviders();
}


UBWebController::~UBWebController()
{
    // NOOP
    if (mStackedWidget) {
        delete mStackedWidget;
    }
}

void UBWebController::initialiazemOEmbedProviders()
{
    mOEmbedProviders << "5min.com";
    mOEmbedProviders << "amazon.com";
    mOEmbedProviders << "flickr";
    mOEmbedProviders << "video.google.";
    mOEmbedProviders << "hulu.com";
    mOEmbedProviders << "imdb.com";
    mOEmbedProviders << "metacafe.com";
    mOEmbedProviders << "qik.com";
    mOEmbedProviders << "slideshare";
    mOEmbedProviders << "5min.com";
    mOEmbedProviders << "twitpic.com";
    mOEmbedProviders << "viddler.com";
    mOEmbedProviders << "vimeo.com";
    mOEmbedProviders << "wikipedia.org";
    mOEmbedProviders << "wordpress.com";
    mOEmbedProviders << "youtube.com";
}

void UBWebController::webBrowserInstance()
{
    QString webHomePage = UBSettings::settings()->webHomePage->get().toString();
    QUrl currentUrl = WBBrowserWindow::guessUrlFromString(webHomePage);

    if (UBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(currentUrl);
    }
    else
    {
        mCurrentWebBrowser = &mWebBrowserList[WebBrowser];
        mToolsCurrentPalette = &mToolsPaletteList[WebBrowser];
        mToolsPalettePositionned = mToolsPalettePositionnedList[WebBrowser];

        if (!(*mCurrentWebBrowser))
        {
            (*mCurrentWebBrowser) = new WBBrowserWindow(mMainWindow->centralWidget(), mMainWindow);

            connect((*mCurrentWebBrowser), SIGNAL(activeViewChange(QWidget*)), this, SLOT(setSourceWidget(QWidget*)));

            WBBrowserWindow::downloadManager()->setParent((*mCurrentWebBrowser), Qt::Tool);

            UBApplication::app()->insertSpaceToToolbarBeforeAction(mMainWindow->webToolBar, mMainWindow->actionBoard, 32);
            UBApplication::app()->decorateActionMenu(mMainWindow->actionMenu);

            bool showAddBookmarkButtons = UBSettings::settings()->webShowAddBookmarkButton->get().toBool();
            mMainWindow->actionBookmarks->setVisible(showAddBookmarkButtons);
            mMainWindow->actionAddBookmark->setVisible(showAddBookmarkButtons);

            mStackedWidget->setCurrentIndex(WebBrowser);
            if (mStackedWidget->currentWidget()) {
                mStackedWidget->removeWidget(mStackedWidget->currentWidget());
            }
            mStackedWidget->insertWidget(WebBrowser, (*mCurrentWebBrowser));

            showTabAtTop(UBSettings::settings()->appToolBarPositionedAtTop->get().toBool());

            adaptToolBar();

            mTrapContentController = new UBTrapWebPageContentController((*mCurrentWebBrowser));

            connect((*mCurrentWebBrowser), SIGNAL(activeViewPageChanged()), this, SLOT(activePageChanged()));

            (*mCurrentWebBrowser)->loadUrl(currentUrl);

            (*mCurrentWebBrowser)->tabWidget()->tabBar()->show();
            (*mCurrentWebBrowser)->tabWidget()->lineEdits()->show();
        }

        mStackedWidget->setCurrentIndex(WebBrowser);
        UBApplication::applicationController->setMirrorSourceWidget((*mCurrentWebBrowser)->paintWidget());
        mMainWindow->switchToWebWidget();

        setupPalettes();
        screenLayoutChanged();

        bool mirroring = UBSettings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();
        UBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(mirroring);
        (*mToolsCurrentPalette)->show();
    }

    if (mDownloadViewIsVisible)
        WBBrowserWindow::downloadManager()->show();
}

void UBWebController::tutorialWebInstance()
{
    QLocale locale = QLocale();
    QString language = "_" + locale.name().left(2);

    QString tutorialHtmlIndexFile = 0;
    QString tutorialPath = "/etc/Tutorial/tutorial" + language + "/index.html";
#if defined(Q_WS_MAC)
    tutorialHtmlIndexFile = QApplication::applicationDirPath()+ "/../Resources" + tutorialPath;
#else
    tutorialHtmlIndexFile = QApplication::applicationDirPath() + tutorialPath;
#endif

    QUrl currentUrl = QUrl::fromLocalFile(tutorialHtmlIndexFile);

    if (UBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(currentUrl);
    }
    else
    {
        mCurrentWebBrowser = &mWebBrowserList[Tutorial];
        mToolsPalettePositionned = &mToolsPalettePositionnedList[Tutorial];

        if (!(*mCurrentWebBrowser))
        {
            (*mCurrentWebBrowser) = new WBBrowserWindow(mMainWindow->centralWidget(), mMainWindow, true);
            connect((*mCurrentWebBrowser), SIGNAL(activeViewChange(QWidget*)), this, SLOT(setSourceWidget(QWidget*)));

            mStackedWidget->setCurrentIndex(Tutorial);
            if (mStackedWidget->currentWidget()) {
                mStackedWidget->removeWidget(mStackedWidget->currentWidget());
            }
            mStackedWidget->insertWidget(Tutorial, (*mCurrentWebBrowser));
            adaptToolBar();

            mTrapContentController = new UBTrapWebPageContentController((*mCurrentWebBrowser));

            connect((*mCurrentWebBrowser), SIGNAL(activeViewPageChanged()), this, SLOT(activePageChanged()));
            (*mCurrentWebBrowser)->loadUrl(currentUrl);

            (*mCurrentWebBrowser)->tabWidget()->tabBar()->hide();
            (*mCurrentWebBrowser)->tabWidget()->lineEdits()->hide();

        }
        else
            (*mCurrentWebBrowser)->loadUrl(currentUrl);

        mStackedWidget->setCurrentIndex(Tutorial);
        UBApplication::applicationController->setMirrorSourceWidget((*mCurrentWebBrowser)->paintWidget());
        mMainWindow->switchToWebWidget();
        screenLayoutChanged();

        bool mirroring = UBSettings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();
        UBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(mirroring);

    }

}

void UBWebController::paraschoolWebInstance()
{
    QLocale locale = QLocale();
    QString language = "_" + locale.name().left(2);
    QString editorPath = "/etc/SankoreEditor/editor" + language + "/index.html";
    QString editorHtmlIndexFile;
#if defined(Q_WS_MAC)
    editorHtmlIndexFile = QApplication::applicationDirPath() + "/../Resources" + editorPath;
#elif defined(Q_WS_WIN)
    editorHtmlIndexFile = QApplication::applicationDirPath() + editorPath;
#else
    editorHtmlIndexFile = QApplication::applicationDirPath() + editorPath;
#endif

    QUrl currentUrl = QUrl::fromLocalFile(editorHtmlIndexFile);

    if (UBSettings::settings()->webUseExternalBrowser->get().toBool()){
        QDesktopServices::openUrl(currentUrl);
    }

    else {
        mCurrentWebBrowser = &mWebBrowserList[Paraschool];
        mToolsCurrentPalette = &mToolsPaletteList[Paraschool];
        mToolsPalettePositionned = &mToolsPalettePositionnedList[Paraschool];
        if (!(*mCurrentWebBrowser)){
            (*mCurrentWebBrowser) = new WBBrowserWindow(mMainWindow->centralWidget(), mMainWindow, true);
            connect((*mCurrentWebBrowser), SIGNAL(activeViewChange(QWidget*)), this, SLOT(setSourceWidget(QWidget*)));

            mStackedWidget->setCurrentIndex(Paraschool);
            if (mStackedWidget->currentWidget()) {
                mStackedWidget->removeWidget(mStackedWidget->currentWidget());
            }
            mStackedWidget->insertWidget(Paraschool, (*mCurrentWebBrowser));

            adaptToolBar();

            mTrapContentController = new UBTrapWebPageContentController((*mCurrentWebBrowser));

            connect((*mCurrentWebBrowser), SIGNAL(activeViewPageChanged()), this, SLOT(activePageChanged()));
            (*mCurrentWebBrowser)->loadUrl(currentUrl);
            (*mCurrentWebBrowser)->tabWidget()->tabBar()->hide();
            (*mCurrentWebBrowser)->tabWidget()->lineEdits()->hide();

        }

        mStackedWidget->setCurrentIndex(Paraschool);
        UBApplication::applicationController->setMirrorSourceWidget((*mCurrentWebBrowser)->paintWidget());
        mMainWindow->switchToWebWidget();
        screenLayoutChanged();

        bool mirroring = UBSettings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();
        UBApplication::mainWindow->actionWebShowHideOnDisplay->setChecked(mirroring);
    }
}


void UBWebController::show(WebInstance type)
{
    switch(type)
    {
    case WebBrowser:
        webBrowserInstance();
        break;
    case Tutorial:
        tutorialWebInstance();
        break;
    case Paraschool:
        paraschoolWebInstance();
        break;
    default:
        qCritical() << __FILE__ << " non supported web instance type " << QString::number(type) ;
        break;
    }
}

void UBWebController::setSourceWidget(QWidget* pWidget)
{
    mBrowserWidget = pWidget;
    UBApplication::applicationController->setMirrorSourceWidget(pWidget);
}

void UBWebController::activePageChanged()
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser)->currentTabWebView())
    {
        if (mTrapContentController && (*mCurrentWebBrowser)->currentTabWebView()->page())
        {
            mTrapContentController->updateTrapContentFromPage((*mCurrentWebBrowser)->currentTabWebView()->page()->currentFrame());
        }

        mMainWindow->actionWebTrap->setChecked(false);

        QUrl latestUrl = (*mCurrentWebBrowser)->currentTabWebView()->url();

        UBApplication::mainWindow->actionEduMedia->setEnabled(isEduMedia(latestUrl));

        emit activeWebPageChanged((*mCurrentWebBrowser)->currentTabWebView());
    }
}

bool UBWebController::hasEmbeddedContent()
{
    bool bHasContent = false;
    if(mCurrentWebBrowser){
        QString html = (*mCurrentWebBrowser)->currentTabWebView()->webPage()->mainFrame()->toHtml();

        // search the presence of "+oembed"
        QString query = "\\+oembed([^>]*)>";
        QRegExp exp(query);
        exp.indexIn(html);
        QStringList results = exp.capturedTexts();
        if(2 <= results.size() && "" != results.at(1)){
            // An embedded content has been found, no need to check the other ones
            bHasContent = true;
        }else{
            QList<QUrl> contentUrls;
            lookForEmbedContent(&html, "embed", "src", &contentUrls);
            lookForEmbedContent(&html, "video", "src", &contentUrls);
            lookForEmbedContent(&html, "object", "data", &contentUrls);

            // TODO: check the hidden iFrame

            if(!contentUrls.empty()){
                bHasContent = true;
            }
        }
    }

    return bHasContent;
}

QPixmap UBWebController::captureCurrentPage()
{
    QPixmap pix;

    if (mCurrentWebBrowser
            && (*mCurrentWebBrowser)
            && (*mCurrentWebBrowser)->currentTabWebView()
            && (*mCurrentWebBrowser)->currentTabWebView()->page()
            && (*mCurrentWebBrowser)->currentTabWebView()->page()->mainFrame())
    {
        QWebFrame* frame = (*mCurrentWebBrowser)->currentTabWebView()->page()->mainFrame();
        QSize size = frame->contentsSize();

        qDebug() << size;

        QVariant top = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientTop");
        QVariant left = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientLeft");
        QVariant width = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientWidth");
        QVariant height = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientHeight");

        QSize vieportSize = (*mCurrentWebBrowser)->currentTabWebView()->page()->viewportSize();
        (*mCurrentWebBrowser)->currentTabWebView()->page()->setViewportSize(frame->contentsSize());
        pix = QPixmap(frame->geometry().width(), frame->geometry().height());

        {
            QPainter p(&pix);
            frame->render(&p);
        }

        if (left.isValid() && top.isValid() && width.isValid() && height.isValid())
        {
            bool okLeft, okTop, okWidth, okHeight;

            int iLeft = left.toInt(&okLeft) * frame->zoomFactor();
            int iTop = top.toInt(&okTop) * frame->zoomFactor();
            int iWidth = width.toInt(&okWidth) * frame->zoomFactor();
            int iHeight = height.toInt(&okHeight) * frame->zoomFactor();

            if(okLeft && okTop && okWidth && okHeight)
            {
                pix = pix.copy(iLeft, iTop, iWidth, iHeight);
            }
        }


        (*mCurrentWebBrowser)->currentTabWebView()->page()->setViewportSize(vieportSize);
    }

    return pix;
}


void UBWebController::setupPalettes()
{
    if(!(*mToolsCurrentPalette))
    {
        (*mToolsCurrentPalette) = new UBWebToolsPalette(UBApplication::mainWindow, false);
        UBApplication::boardController->paletteManager()->setCurrentWebToolsPalette(*mToolsCurrentPalette);
#ifndef Q_WS_WIN
        if (UBPlatformUtils::hasVirtualKeyboard() && UBApplication::boardController->paletteManager()->mKeyboardPalette)
            connect(UBApplication::boardController->paletteManager()->mKeyboardPalette, SIGNAL(closed()),
                    UBApplication::boardController->paletteManager()->mKeyboardPalette, SLOT(onDeactivated()));
#endif

        connect(mMainWindow->actionWebCustomCapture, SIGNAL(triggered()), this, SLOT(customCapture()));
        connect(mMainWindow->actionWebWindowCapture, SIGNAL(triggered()), this, SLOT(captureWindow()));
        connect(mMainWindow->actionEduMedia, SIGNAL(triggered()), this, SLOT(captureEduMedia()));

        connect(mMainWindow->actionWebShowHideOnDisplay, SIGNAL(toggled(bool)), this, SLOT(toogleMirroring(bool)));
       // connect(mMainWindow->actionWebTrap, SIGNAL(toggled(bool)), this, SLOT(toggleWebTrap(bool)));
        connect(mMainWindow->actionWebTrapContent, SIGNAL(triggered()), this, SLOT(webTrapContent()));

        (*mToolsCurrentPalette)->hide();
        (*mToolsCurrentPalette)->adjustSizeAndPosition();

        if (controlView()){
            int left = controlView()->width() - 20 - (*mToolsCurrentPalette)->width();
            int top = (controlView()->height() - (*mToolsCurrentPalette)->height()) / 2;
            mToolsPalettePositionnedList[mStackedWidget->currentIndex()] = true;
            (*mToolsCurrentPalette)->setCustomPosition(true);
            (*mToolsCurrentPalette)->move(left, top);
        }
        mMainWindow->actionWebTools->trigger();
    }
}


void UBWebController::toggleWebTrap(bool checked)
{
    if (mCurrentWebBrowser
            && (*mCurrentWebBrowser)
            && (*mCurrentWebBrowser)->currentTabWebView())
    {
        (*mCurrentWebBrowser)->currentTabWebView()->setIsTrapping(checked);
    }
}

void UBWebController::webTrapContent()
{
    mTrapContentController->showTrapContent();
    activePageChanged();
}

void UBWebController::toggleWebToolsPalette(bool checked)
{
    (*mToolsCurrentPalette)->setVisible(checked);
}


void UBWebController::captureWindow()
{
    QPixmap webPagePixmap = captureCurrentPage();

    if (!webPagePixmap.isNull())
    {
        emit imageCaptured(webPagePixmap, true, (*mCurrentWebBrowser)->currentTabWebView()->url());
    }
}


void UBWebController::customCapture()
{
    (*mToolsCurrentPalette)->setVisible(false);
    qApp->processEvents();

    UBCustomCaptureWindow customCaptureWindow((*mCurrentWebBrowser));

    customCaptureWindow.show();

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted)
    {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false, (*mCurrentWebBrowser)->currentTabWebView()->url());
    }

    (*mToolsCurrentPalette)->setVisible(true);
}


void UBWebController::toogleMirroring(bool checked)
{
    UBApplication::applicationController->mirroringEnabled(checked);
}


QPixmap UBWebController::getScreenPixmap()
{
    QDesktopWidget *desktop = QApplication::desktop();
    // we capture the screen in which the mouse is.
    const QRect primaryScreenRect = desktop->screenGeometry(QCursor::pos());
    QCoreApplication::flush ();

    return QPixmap::grabWindow(desktop->winId(),
                               primaryScreenRect.x(), primaryScreenRect.y(),
                               primaryScreenRect.width(), primaryScreenRect.height());
}


void UBWebController::screenLayoutChanged()
{
    bool hasDisplay = (UBApplication::applicationController &&
                       UBApplication::applicationController->displayManager() &&
                       UBApplication::applicationController->displayManager()->hasDisplay());

    UBApplication::mainWindow->actionWebShowHideOnDisplay->setVisible(hasDisplay);
}


void UBWebController::closing()
{
    //NOOP
}


void UBWebController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->actionWebReload->setVisible(highResolution);
    mMainWindow->actionStopLoading->setVisible(highResolution);

    if(mCurrentWebBrowser && (*mCurrentWebBrowser) )
        (*mCurrentWebBrowser)->adaptToolBar(highResolution);

}


void UBWebController::showTabAtTop(bool attop)
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser))
        (*mCurrentWebBrowser)->showTabAtTop(attop);
}

void UBWebController::captureoEmbed(QUrl currentUrl)
{
    if (isOEmbedable(currentUrl))
    {
        UBGraphicsW3CWidgetItem * widget = UBApplication::boardController->activeScene()->addOEmbed(currentUrl);

        if(widget)
        {
            UBApplication::applicationController->showBoard();
            UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);
        }
    }
}

void UBWebController::captureoEmbed()
{
    if ( mCurrentWebBrowser  && (*mCurrentWebBrowser) && (*mCurrentWebBrowser)->currentTabWebView()){
        // TODO : Uncomment the next lines to continue the youtube button bugfix
        //    getEmbeddableContent();

        // And comment from here

        QWebView* webView = (*mCurrentWebBrowser)->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isOEmbedable(currentUrl))
        {
            UBGraphicsW3CWidgetItem * widget = UBApplication::boardController->activeScene()->addOEmbed(currentUrl);

            if(widget)
            {
                UBApplication::applicationController->showBoard();
                UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);
            }
        }
        // --> Until here
    }
}

void UBWebController::lookForEmbedContent(QString* pHtml, QString tag, QString attribute, QList<QUrl> *pList)
{
    if(NULL != pHtml && NULL != pList){
        QVector<QString> urlsFound;
        // Check for <embed> content
        QRegExp exp(QString("<%0(.*)").arg(tag));
        exp.indexIn(*pHtml);
        QStringList strl = exp.capturedTexts();
        if(2 <= strl.size() && strl.at(1) != ""){
            // Here we call this regular expression:
            // src\s?=\s?['"]([^'"]*)['"]
            // It says: give me all characters that are after src=" (or src = ")
            QRegExp src(QString("%0\\s?=\\s?['\"]([^'\"]*)['\"]").arg(attribute));
            for(int i=1; i<strl.size(); i++){
                src.indexIn(strl.at(i));
                QStringList urls = src.capturedTexts();
                if(2 <= urls.size() && urls.at(1) != "" && !urlsFound.contains(urls.at(1))){
                    urlsFound << urls.at(1);
                    pList->append(QUrl(urls.at(1)));
                }
            }
        }
    }
}

void UBWebController::checkForOEmbed(QString *pHtml)
{
    mOEmbedParser.parse(*pHtml);
}

void UBWebController::getEmbeddableContent()
{
    // Get the source code of the page
    if(mCurrentWebBrowser){
        QNetworkAccessManager* pNam = (*mCurrentWebBrowser)->currentTabWebView()->webPage()->networkAccessManager();
        if(NULL != pNam){
            QString html = (*mCurrentWebBrowser)->currentTabWebView()->webPage()->mainFrame()->toHtml();
            mOEmbedParser.setNetworkAccessManager(pNam);

            // First, we have to check if there is some oembed content
            checkForOEmbed(&html);

            // Note: The other contents will be verified once the oembed ones have been checked
        }
    }
}

void UBWebController::captureEduMedia()
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser)
            && (*mCurrentWebBrowser)->currentTabWebView())
    {
        QWebView* webView = (*mCurrentWebBrowser)->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isEduMedia(currentUrl))
        {
            QWebElementCollection objects = webView->page()->currentFrame()->findAllElements("object");

            foreach(QWebElement object, objects)
            {
                foreach(QWebElement param, object.findAll("param"))
                {
                    if(param.attribute("name") == "flashvars")
                    {
                        QString value = param.attribute("value");
                        QString midValue;
                        QString langValue;
                        QString hostValue;

                        QStringList flashVars = value.split("&");

                        foreach(QString flashVar, flashVars)
                        {
                            QStringList var = flashVar.split("=");

                            if (var.length() < 2)
                                break;

                            if (var.at(0) == "mid")
                                midValue = var.at(1);
                            else if (var.at(0) == "lang")
                                langValue = var.at(1);
                            else if (var.at(0) == "host")
                                hostValue = var.at(1);

                        }

                        if (midValue.length() > 0 && langValue.length() > 0 && hostValue.length() > 0)
                        {
                            QString swfUrl = "http://" + hostValue + "/" + langValue + "/fl/" + midValue;

                            UBApplication::boardController->downloadURL(QUrl(swfUrl));

                            UBApplication::applicationController->showBoard();
                            UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);

                            return;
                        }
                    }
                }
            }
        }
    }
    else
    {
        UBApplication::showMessage("Cannot find any reference to eduMedia content");
    }
}

bool UBWebController::isOEmbedable(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    foreach(QString provider, mOEmbedProviders)
    {
        if(urlAsString.contains(provider))
        {
            if ("youtube.com" == provider)
                return (urlAsString.contains("/embed/") || urlAsString.contains("watch?v="));
            else
                return true;
        }
    }

    return false;
}


bool UBWebController::isEduMedia(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    if (urlAsString.contains("edumedia-sciences.com"))
    {
        return true;
    }

    return false;
}


void UBWebController::loadUrl(const QUrl& url)
{
    bool webBrowserAlreadyInstanciated = dynamic_cast<WBBrowserWindow*>(mStackedWidget->widget(WebBrowser)) != NULL;
    UBApplication::applicationController->showInternet();
    if (UBSettings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(url);
    }
    else
    {
        if (!webBrowserAlreadyInstanciated) {
            (*mCurrentWebBrowser)->loadUrl(url);
        }
        else {
            (*mCurrentWebBrowser)->loadUrlInNewTab(url);
        }
    }
}


QWebView* UBWebController::createNewTab()
{
    if (mCurrentWebBrowser && !(*mCurrentWebBrowser))
    {
        UBApplication::applicationController->showInternet();
    }

    return (*mCurrentWebBrowser)->createNewTab();
}

QUrl UBWebController::currentPageUrl() const
{
    return (*mCurrentWebBrowser)->currentTabWebView()->url();
}


void UBWebController::copy()
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser) && (*mCurrentWebBrowser)->currentTabWebView())
    {
        QWebView* webView = (*mCurrentWebBrowser)->currentTabWebView();
        QAction *act = webView->pageAction(QWebPage::Copy);
        if(act)
            act->trigger();
    }
}


void UBWebController::paste()
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser) && (*mCurrentWebBrowser)->currentTabWebView())
    {
        QWebView* webView = (*mCurrentWebBrowser)->currentTabWebView();
        QAction *act = webView->pageAction(QWebPage::Paste);
        if(act)
            act->trigger();
    }
}


void UBWebController::onActionBookmark()
{
    QString title = (*mCurrentWebBrowser)->currentTabWebView()->page()->mainFrame()->title();
    QString url = (*mCurrentWebBrowser)->currentTabWebView()->page()->mainFrame()->url().toString();
    if(title.isEmpty())
        title = url;
    UBApplication::boardController->paletteManager()->featuresWidget()->createBookmark(title, url);
}

void UBWebController::cut()
{
    if (mCurrentWebBrowser && (*mCurrentWebBrowser) && (*mCurrentWebBrowser)->currentTabWebView())
    {
        QWebView* webView = (*mCurrentWebBrowser)->currentTabWebView();
        QAction *act = webView->pageAction(QWebPage::Cut);
        if(act)
            act->trigger();
    }
}

void UBWebController::onOEmbedParsed(QVector<sOEmbedContent> contents)
{
    QList<QUrl> urls;

    foreach(sOEmbedContent cnt, contents){
        urls << QUrl(cnt.url);
    }

    // TODO : Implement this
    //lookForEmbedContent(&html, "embed", "src", &urls);
    //lookForEmbedContent(&html, "video", "src", &contentUrls);
    //lookForEmbedContent(&html, "object", "data", &contentUrls);

    // TODO: check the hidden iFrame

    if(!urls.empty()){
        QUrl contentUrl;    // The selected content url

        if(1 == urls.size()){
            contentUrl = urls.at(0);
        }else{
            // TODO : Display a dialog box asking the user which content to get and set contentUrl to the selected content

        }

        UBGraphicsW3CWidgetItem * widget = UBApplication::boardController->activeScene()->addOEmbed(contentUrl);

        if(widget)
        {
            UBApplication::applicationController->showBoard();
            UBDrawingController::drawingController()->setStylusTool(UBStylusTool::Selector);
        }
    }
}
