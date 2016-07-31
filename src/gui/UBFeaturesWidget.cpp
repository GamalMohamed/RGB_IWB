#include <QDomDocument>
#include <QWebView>

#include "UBFeaturesWidget.h"
#include "gui/UBThumbnailWidget.h"
#include "frameworks/UBFileSystemUtils.h"
#include "core/UBApplication.h"
#include "core/UBDownloadManager.h"
#include "globals/UBGlobals.h"
#include "board/UBBoardController.h"
#include "board/UBBoardPaletteManager.h" // Issue 1684 - CFA - 20131120
#include "gui/UBMainWindow.h"
#include "web/UBWebController.h"

const char *UBFeaturesWidget::objNamePathList = "PathList";
const char *UBFeaturesWidget::objNameFeatureList = "FeatureList";

const QMargins FeatureListMargins(0, 0, 0, 30);
const int FeatureListBorderOffset = 10;
const char featureTypeSplitter = ':';
static const QString mimeSankoreFeatureTypes = "Sankore/featureTypes";

UBFeaturesWidget::UBFeaturesWidget(QWidget *parent, const char *name)
    : UBDockPaletteWidget(parent)
    , imageGatherer(NULL)
{
    setObjectName(name);
    mName = "FeaturesWidget";
    mVisibleState = true;

    SET_STYLE_SHEET();

    setAcceptDrops(true);

    //Main UBFeature functionality
    controller = new UBFeaturesController(this);

    //Main layout including all the widgets in palette
    layout = new QVBoxLayout(this);

    //Path icon view on the top of the palette
    pathListView = new UBFeaturesListView(this, objNamePathList);
    controller->assignPathListView(pathListView);

    centralWidget = new UBFeaturesCentralWidget(this);
    centralWidget->setStyleSheet("background-color: rgb(180,0,255);"); //change
    controller->assignFeaturesListView(centralWidget->listView());
    centralWidget->setSliderPosition(UBSettings::settings()->featureSliderPosition->get().toInt());

    //Bottom actionbar for DnD, quick search etc
    mActionBar = new UBFeaturesActionBar(controller, this);

    //Filling main layout
    layout->addWidget(pathListView);
    layout->addWidget(centralWidget);
    layout->addWidget(mActionBar);


    connect(centralWidget->listView(), SIGNAL(restoreFeature(QVector<UBFeature>)), controller, SLOT(restoreFeature(QVector<UBFeature>)));
    connect(this, SIGNAL(allowNewFolderButton(bool)), mActionBar, SLOT(allowNewFolderBtn(bool)));
    connect(this, SIGNAL(allowDeleteButton(bool)), mActionBar, SLOT(allowDeleteButton(bool)));
    connect(mActionBar, SIGNAL(deleteSelectedElements()), this, SLOT(deleteSelectedElements()));
    connect(mActionBar, SIGNAL(newFolderToCreate()), this, SLOT(createNewFolder()));
    connect(centralWidget->listView(), SIGNAL(clicked(QModelIndex)),
            this, SLOT(currentSelected(QModelIndex)));
    connect(centralWidget->listView()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection))
            , this, SLOT(processViewSelectionChanged(QItemSelection,QItemSelection)));
    connect(this, SIGNAL(sendFileNameList(QStringList)),
            centralWidget, SIGNAL(sendFileNameList(QStringList)));
    connect(mActionBar, SIGNAL(searchElement(const QString &)),
            this, SLOT( searchStarted(const QString &)));
    connect(mActionBar, SIGNAL(deleteElements(const UBFeaturesMimeData *)),
            this, SLOT(deleteElements(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(addToFavorite(const UBFeaturesMimeData *)),
            this, SLOT(addToFavorite(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(removeFromFavorite(const UBFeaturesMimeData *)),
            this, SLOT(removeFromFavorite(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(addElementsToFavorite() ),
            this, SLOT ( addElementsToFavorite()) );
    connect(mActionBar, SIGNAL(removeElementsFromFavorite()),
            this, SLOT (removeElementsFromFavorite()));

    connect(mActionBar, SIGNAL(rescanModel()), this, SLOT(rescanModel()));
    connect(centralWidget, SIGNAL(lockMainWidget(bool)), this, SLOT(lockIt(bool)));
    connect(controller, SIGNAL(scanStarted()), centralWidget, SLOT(scanStarted()));
    connect(controller, SIGNAL(scanFinished()), centralWidget, SLOT(scanFinished()));
    connect(controller, SIGNAL(scanStarted()), mActionBar, SLOT(lockIt()));
    connect(controller, SIGNAL(scanFinished()), mActionBar, SLOT(unlockIt()));
    connect(pathListView, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(currentSelected(const QModelIndex &)));
    connect(UBApplication::boardController, SIGNAL(displayMetadata(QMap<QString,QString>)),
            this, SLOT(onDisplayMetadata( QMap<QString,QString>)));
    connect(UBDownloadManager::downloadManager(), SIGNAL( addDownloadedFileToLibrary( bool, QUrl, QString, QByteArray, QString))
             , this, SLOT(onAddDownloadedFileToLibrary(bool, QUrl, QString,QByteArray, QString)));
    connect(centralWidget, SIGNAL(createNewFolderSignal(QString)),
            controller, SLOT(addNewFolder(QString)));
    connect(controller, SIGNAL(maxFilesCountEvaluated(int)), centralWidget,
            SIGNAL(maxFilesCountEvaluated(int)));
    connect(controller, SIGNAL(featureAddedFromThread()), centralWidget,
            SIGNAL(increaseStatusBarValue()));
    connect(controller, SIGNAL(scanCategory(QString)),
            centralWidget, SIGNAL(scanCategory(QString)));
    connect(controller, SIGNAL(scanPath(QString)),
            centralWidget, SIGNAL(scanPath(QString)));
}

UBFeaturesWidget::~UBFeaturesWidget()
{
    if (NULL != imageGatherer)
        delete imageGatherer;
}

void UBFeaturesWidget::searchStarted(const QString &pattern)
{
    controller->searchStarted(pattern, centralWidget->listView());
}

void UBFeaturesWidget::currentSelected(const QModelIndex &current)
{
    if (!current.isValid()) {
        qWarning() << "SLOT:currentSelected, invalid index catched";
        return;
    }

    QString objName = sender()->objectName();

    if (objName.isEmpty()) {
        qWarning() << "incorrect sender";
    } else if (objName == objNamePathList) {
        //Calling to reset the model for listView. Maybe separate function needed
        controller->searchStarted("", centralWidget->listView());
    }

    UBFeature feature = controller->getFeature(current, objName);

    //issue 1474 - NNE - 20131125
    bool isNotTrash = feature != controller->trashData.categoryFeature();
    bool isInTrash = feature.inTrash();

    //if the folder is in the trash, do nothing
    if(feature.isFolder() && isNotTrash && isInTrash) return;
    //issue 1474 - NNE - 20131125 : END

    if ( feature.isFolder()) {
        QString newPath = feature.getFullVirtualPath();

        controller->setCurrentElement(feature);
        controller->siftElements(newPath);

        centralWidget->switchTo(UBFeaturesCentralWidget::MainList);

        if ( feature.getType() == FEATURE_FAVORITE ) {
            mActionBar->setCurrentState( IN_FAVORITE );

        }  else if ( feature.getType() == FEATURE_CATEGORY && feature.getName() == "root" ) {
            mActionBar->setCurrentState( IN_ROOT );

        } else if (feature.getType() == FEATURE_TRASH) {
            mActionBar->setCurrentState(IN_TRASH);

        } else if (feature.getType() == FEATURE_SEARCH) {
            //The search feature behavior is not standard. If features list clicked - show empty element
            //else show existing saved features search QWebView
            if (sender()->objectName() == objNameFeatureList) {
                centralWidget->showElement(feature, UBFeaturesCentralWidget::FeaturesWebView);
            } else if (sender()->objectName() == objNamePathList) {
                centralWidget->switchTo(UBFeaturesCentralWidget::FeaturesWebView);
            }

        } else  {
            mActionBar->setCurrentState(IN_FOLDER);
        }

    } else {
        if(feature.getType() == FEATURE_BOOKMARK){
            QString url;
            QFile bookmarkFile(feature.getFullPath().toLocalFile());
            if(bookmarkFile.open(QIODevice::ReadOnly|QIODevice::Text)){
            url = QString::fromUtf8(bookmarkFile.readAll());
            bookmarkFile.close();
            UBApplication::webController->loadUrl(QUrl(url));
            }
            else
                qWarning() << "failed to read file named " << feature.getFullPath().toLocalFile();
        }
        else{
            centralWidget->showElement(feature, UBFeaturesCentralWidget::FeaturePropertiesList);
            mActionBar->setCurrentState( IN_PROPERTIES );
        }
    }

    mActionBar->cleanText();
    emit allowNewFolderButton(controller->newFolderAllowed());

}

void UBFeaturesWidget::createNewFolder()
{
    if (controller->newFolderAllowed()) {
        centralWidget->showAdditionalData(UBFeaturesCentralWidget::NewFolderDialog, UBFeaturesCentralWidget::Modal);
        emit sendFileNameList(controller->getFileNamesInFolders());
    }
}

void UBFeaturesWidget::deleteElements( const UBFeaturesMimeData * mimeData )
{
    if (!mimeData->features().count() )
        return;

    QList<UBFeature> featuresList = mimeData->features();

    foreach(UBFeature curFeature, featuresList){
        if(curFeature.inTrash()){
            //issue 1474 - NNE - 20131120
            controller->removeFromTrashRegistery(curFeature);
            controller->deleteItem(curFeature.getFullPath());
        }else{
           controller->moveToTrash(curFeature);
        }
    }

    controller->refreshModels();
}

void UBFeaturesWidget::deleteSelectedElements()
{
    QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();

    QList<UBFeature> featureasToMove;
    for (int i = 0; i < selected.count(); i++)
    {
        featureasToMove.append(controller->getFeature(selected.at(i), objNameFeatureList));
    }

    foreach (UBFeature feature, featureasToMove)
    {
        if (feature.isDeletable()) {
            if (feature.inTrash()) {
                controller->deleteItem(feature);
            } else {
                controller->moveToTrash(feature, true);
            }
        }
    }

    controller->refreshModels();
}

void UBFeaturesWidget::rescanModel()
{
    controller->rescanModel();
}

void UBFeaturesWidget::lockIt(bool pLock)
{
    mActionBar->setEnabled(!pLock);
    pathListView->setEnabled(!pLock);
    centralWidget->setLockedExcludingAdditional(pLock);
}

void UBFeaturesWidget::processViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    bool selectedDeletable = true;

    if (!selected.indexes().count()) {
        selectedDeletable = false;
    } else {
        foreach (QModelIndex curIndex, selected.indexes()) {
            UBFeature curFeature = curIndex.data(Qt::UserRole + 1).value<UBFeature>();
            if (!curFeature.isDeletable()) {
                selectedDeletable = false;
                break;
            }
        }
    }

    emit allowDeleteButton(selectedDeletable);
}

void UBFeaturesWidget::addToFavorite( const UBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();
    foreach ( QUrl url, urls ) {
        controller->addToFavorite(url);
    }

    controller->refreshModels();
}

void UBFeaturesWidget::removeFromFavorite( const UBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();

    foreach( QUrl url, urls ) {
        controller->removeFromFavorite(url);
    }
}

void UBFeaturesWidget::onDisplayMetadata( QMap<QString,QString> metadata )
{
    QString previewImageUrl = ":images/libpalette/notFound.png";

    QString widgetsUrl = QUrl::fromEncoded(metadata["Url"].toAscii()).toString();
    QString widgetsThumbsUrl = QUrl::fromEncoded(metadata["thumbnailUrl"].toAscii()).toString();

    if(!QFileInfo(metadata["Title"]).suffix().length() && QFileInfo(widgetsUrl).suffix().length())
        metadata.insert("Title",metadata["Title"] + "." + QFileInfo(widgetsUrl).suffix());

    QString strType = UBFileSystemUtils::mimeTypeFromFileName(widgetsUrl);
    UBMimeType::Enum thumbType = UBFileSystemUtils::mimeTypeFromString(strType);

    switch (static_cast<int>(thumbType)) {
    case UBMimeType::Audio:
        previewImageUrl = ":images/libpalette/soundIcon.svg";
        break;

    case UBMimeType::Video:
        previewImageUrl = ":images/libpalette/movieIcon.svg";
        break;

    case UBMimeType::Flash:
        previewImageUrl = ":images/libpalette/FlashIcon.svg";
        break;

    case UBMimeType::RasterImage:
    case UBMimeType::VectorImage:
        previewImageUrl = widgetsUrl;
        break;
    }

    if (!widgetsThumbsUrl.isNull()) {
        previewImageUrl = ":/images/libpalette/loading.png";
        if (!imageGatherer)
            imageGatherer = new UBDownloadHttpFile(0, this);

        connect(imageGatherer, SIGNAL(downloadFinished(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)), this, SLOT(onPreviewLoaded(int, bool, QUrl, QUrl, QString, QByteArray, QPointF, QSize, bool)));

        // We send here the request and store its reply in order to be able to cancel it if needed
        imageGatherer->get(QUrl(widgetsThumbsUrl), QPoint(0,0), QSize(), false);
    }

    UBFeature feature( "/root", QImage(previewImageUrl), QString(), widgetsUrl, FEATURE_ITEM );
    feature.setMetadata( metadata );

    centralWidget->showElement(feature, UBFeaturesCentralWidget::FeaturePropertiesList);
    mActionBar->setCurrentState( IN_PROPERTIES );
}


void UBFeaturesWidget::onPreviewLoaded(int id, bool pSuccess, QUrl sourceUrl, QUrl originalUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
{
    Q_UNUSED(id);
    Q_UNUSED(pSuccess);
    Q_UNUSED(originalUrl);
    Q_UNUSED(isBackground);
    Q_UNUSED(pSize);
    Q_UNUSED(pPos);
    Q_UNUSED(sourceUrl);
    Q_UNUSED(pContentTypeHeader)

    QImage img;
    img.loadFromData(pData);
    QPixmap pix = QPixmap::fromImage(img);
    centralWidget->setPropertiesPixmap(pix);
    centralWidget->setPropertiesThumbnail(pix);
}

void UBFeaturesWidget::onAddDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentHeader, QByteArray pData, QString pTitle)
{
    if (pSuccess) {
        controller->addDownloadedFile(sourceUrl, pData, pContentHeader, pTitle);
        controller->refreshModels();
    }
}

void UBFeaturesWidget::addElementsToFavorite()
{
    if ( centralWidget->currentView() == UBFeaturesCentralWidget::FeaturePropertiesList ) {
        UBFeature feature = centralWidget->getCurElementFromProperties();
        if ( feature != UBFeature() && !UBApplication::isFromWeb(feature.getFullPath().toString())) {
            controller->addToFavorite( feature.getFullPath() );
        }

    } else if ( centralWidget->currentView() == UBFeaturesCentralWidget::MainList ) {
        QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();
        for ( int i = 0; i < selected.size(); ++i ) {
            UBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<UBFeature>();
            if (feature.getType() != FEATURE_FOLDER) {
                controller->addToFavorite(feature.getFullPath());
            }
       }
    }

    controller->refreshModels();
}

void UBFeaturesWidget::removeElementsFromFavorite()
{
    QModelIndexList selected = centralWidget->listView()->selectionModel()->selectedIndexes();
    QList <QUrl> items;
    for ( int i = 0; i < selected.size(); ++i )  {
        UBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<UBFeature>();
        items.append( feature.getFullPath() );
    }

    foreach ( QUrl url, items )  {
        controller->removeFromFavorite(url, true);
    }

    controller->refreshModels();
}

void UBFeaturesWidget::switchToListView()
{
//	stackedWidget->setCurrentIndex(ID_LISTVIEW);
//	currentStackedWidget = ID_LISTVIEW;
}

void UBFeaturesWidget::switchToProperties()
{
//	stackedWidget->setCurrentIndex(ID_PROPERTIES);
//	currentStackedWidget = ID_PROPERTIES;
}

void UBFeaturesWidget::switchToWebView()
{
//	stackedWidget->setCurrentIndex(ID_WEBVIEW);
//	currentStackedWidget = ID_WEBVIEW;
}

QStringList UBFeaturesMimeData::formats() const
{
    return QMimeData::formats();
}

void UBFeaturesWidget::importImage(const QImage &image, const QString &fileName)
{
    controller->importImage(image, fileName);
}

void UBFeaturesWidget::createBookmark(const QString& title, const QString& urlString)
{
    controller->createBookmark(title,urlString);
}

void UBFeaturesWidget::createLink(QString title, QString& urlString,QSize& size, QString mimeType, QString embedCode)
{
    controller->createLink(title,urlString,size,mimeType,embedCode);
}

QString UBFeaturesWidget::importFromUrl(const QUrl &url) const
{
    return controller->moveExternalData(url,UBFeature());
}

void UBFeaturesWidget::switchToElement(const UBFeature &feature)
{
    QString newPath = feature.getFullVirtualPath();
    controller->setCurrentElement(feature);
    controller->siftElements(newPath);

    centralWidget->switchTo(UBFeaturesCentralWidget::MainList);
}

void UBFeaturesWidget::switchToRoot()
{
    switchToElement(controller->getRootElement());
}

void UBFeaturesWidget::switchToBookmarks()
{
    switchToElement(controller->getBookmarkElement());
}

UBFeaturesListView::UBFeaturesListView( QWidget* parent, const char* name )
    : QListView(parent)
{
    setObjectName(name);

    //issue 1474 - NNE - 20131118
    contextMenuTrash.setParent(this);
    contextMenuTrash.setClosable(true);

    textRestoreOneFile = tr("Restore element");
    textRestoreManyFile = tr("Restore %1 elements");

    restoreAction = new QAction(textRestoreOneFile, 0);
    contextMenuTrash.addAction(restoreAction);
    contextMenuTrash.hide();

    timer.setInterval(1000);
    timer.setSingleShot(true);

    connect(&timer, SIGNAL(timeout()), this, SLOT(onLongPressEvent()));

    connect(restoreAction, SIGNAL(triggered()), this, SLOT(emitRestoreFeature()));
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(hideContextMenu()));
    //issue 1474 - NNE - 20131118 : END
}

void UBFeaturesListView::dragEnterEvent( QDragEnterEvent *event )
{
    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() )
        event->acceptProposedAction();
}

void UBFeaturesListView::dragMoveEvent( QDragMoveEvent *event )
{
    //issue 1474 - NNE - 20131120
    timer.stop();

    const UBFeaturesMimeData *fMimeData = qobject_cast<const UBFeaturesMimeData*>(event->mimeData());
    QModelIndex index = indexAt(event->pos());
    UBFeature onFeature = model()->data(index, Qt::UserRole + 1).value<UBFeature>();
    if (fMimeData) {
        if (!index.isValid()
                || !onFeature.isFolder()
                || !onFeature.testPermissions(UBFeature::WRITE_P)) {
            event->ignore();
            return;
        }
        foreach (UBFeature curFeature, fMimeData->features()) {
            if (curFeature == onFeature) {
                event->ignore();
                return;
            }
        }
     }

    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() ) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void UBFeaturesListView::dropEvent( QDropEvent *event )
{
    QWidget *eventSource = event->source();
    if (eventSource && eventSource->objectName() == UBFeaturesWidget::objNameFeatureList) {
        event->setDropAction( Qt::MoveAction );
    }

    QListView::dropEvent( event );
}

void UBFeaturesListView::thumbnailSizeChanged( int value )
{
    setIconSize(QSize(value, value));
    setGridSize(QSize(value + 20, value + 20 ));

    UBSettings::settings()->featureSliderPosition->set(value);
}

//issue 1474 - NNE - 20131118 : Add the menu for restoring the elements in the trash
void UBFeaturesListView::mousePressEvent(QMouseEvent *event)
{
    if(this->objectName() == UBFeaturesWidget::objNameFeatureList){
        QModelIndex index = this->indexAt(event->pos());
        UBFeature feature = model()->data(index, Qt::UserRole + 1).value<UBFeature>();

        if(feature.inTrash() && feature.getType() != FEATURE_TRASH){
            contextMenuTrash.move(event->pos());
            timer.start();
        }
    }

    QListView::mousePressEvent(event);
}

void UBFeaturesListView::mouseReleaseEvent(QMouseEvent *event)
{
    timer.stop();

    //if the context menu is not visible, do the normale process
    //else do nothing (to avoid to show the menu of the feature)
    if(!contextMenuTrash.isVisible()){
        QListView::mouseReleaseEvent(event);
    }
}

void UBFeaturesListView::onLongPressEvent()
{
    QModelIndexList selectedModel = this->selectionModel()->selectedIndexes();

    if(selectedModel.count() == 1){
        restoreAction->setText(textRestoreOneFile);
    }else{
        restoreAction->setText(textRestoreManyFile.arg(selectedModel.count()));
    }

    contextMenuTrash.show();
}

void UBFeaturesListView::emitRestoreFeature()
{
    QModelIndexList selectedModel = this->selectionModel()->selectedIndexes();
    QVector<UBFeature> selectedFeatures;

    foreach (QModelIndex m, selectedModel) {
        selectedFeatures.push_back(model()->data(m, Qt::UserRole + 1).value<UBFeature>());
    }

    emit restoreFeature(selectedFeatures);
}

void UBFeaturesListView::hideContextMenu()
{
    this->contextMenuTrash.hide();
}

UBFeaturesNavigatorWidget::UBFeaturesNavigatorWidget(QWidget *parent, const char *name) :
    QWidget(parent), mListView(0), mListSlider(0)

{
    name = "UBFeaturesNavigatorWidget";

    setObjectName(name);
//    SET_STYLE_SHEET()


    mListView = new UBFeaturesListView(this, UBFeaturesWidget::objNameFeatureList);

    mListSlider = new QSlider(Qt::Horizontal, this);

    mListSlider->setMinimum(UBFeaturesWidget::minThumbnailSize);
    mListSlider->setMaximum(UBFeaturesWidget::maxThumbnailSize);
    mListSlider->setValue(UBFeaturesWidget::minThumbnailSize);
    mListSlider->setMinimumHeight(20);

    mListView->setParent(this);

    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mainLayer->addWidget(mListView, 1);
    mainLayer->addWidget(mListSlider, 0);
    mainLayer->setMargin(0);

    connect(mListSlider, SIGNAL(valueChanged(int)), mListView, SLOT(thumbnailSizeChanged(int)));
}

void UBFeaturesNavigatorWidget::setSliderPosition(int pValue)
{
    mListSlider->setValue(pValue);
}

UBFeaturesCentralWidget::UBFeaturesCentralWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName("UBFeaturesCentralWidget");
    SET_STYLE_SHEET();

    QVBoxLayout *mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    //Maintains the view of the main part of the palette. Consists of
    //mNavigator
    //featureProperties
    //webVeiw
    mStackedWidget = new QStackedWidget(this);

    //Main features icon view with QSlider on the bottom
    mNavigator = new UBFeaturesNavigatorWidget(this);

    //Specifies the properties of a standalone element
    mFeatureProperties = new UBFeatureProperties(this);

    //Used to show search bar on the search widget
    webView = new UBFeaturesWebView(this);

    //filling stackwidget
    mStackedWidget->addWidget(mNavigator);
    mStackedWidget->addWidget(mFeatureProperties);
    mStackedWidget->addWidget(webView);
    mStackedWidget->setCurrentIndex(MainList);
    mStackedWidget->setContentsMargins(0, 0, 0, 0);


    mAdditionalDataContainer = new QStackedWidget(this);
    mAdditionalDataContainer->setObjectName("mAdditionalDataContainer");

    //New folder dialog
    UBFeaturesNewFolderDialog *dlg = new UBFeaturesNewFolderDialog(mAdditionalDataContainer);
    mAdditionalDataContainer->addWidget(dlg);
    mAdditionalDataContainer->setCurrentIndex(NewFolderDialog);

    connect(dlg, SIGNAL(createNewFolder(QString)), this, SLOT(createNewFolderSlot(QString)));
    connect(dlg, SIGNAL(closeDialog()), this, SLOT(hideAdditionalData()));
    connect(this, SIGNAL(sendFileNameList(QStringList)), dlg, SLOT(setFileNameList(QStringList)));

    //Progress bar to show scanning progress
    UBFeaturesProgressInfo *progressBar = new UBFeaturesProgressInfo();
    mAdditionalDataContainer->addWidget(progressBar);
    mAdditionalDataContainer->setCurrentIndex(ProgressBarWidget);

    connect(this, SIGNAL(maxFilesCountEvaluated(int)), progressBar, SLOT(setProgressMax(int)));
    connect(this, SIGNAL(increaseStatusBarValue()), progressBar, SLOT(increaseProgressValue()));
    connect(this, SIGNAL(scanCategory(QString)), progressBar, SLOT(setCommmonInfoText(QString)));
    connect(this, SIGNAL(scanPath(QString)), progressBar, SLOT(setDetailedInfoText(QString)));

    mLayout->addWidget(mStackedWidget, 1);
    mLayout->addWidget(mAdditionalDataContainer, 0);

    mAdditionalDataContainer->hide();
}

void UBFeaturesCentralWidget::showElement(const UBFeature &feature, StackElement pView)
{
    if (pView == FeaturesWebView) {
        webView->showElement(feature);
        mStackedWidget->setCurrentIndex(FeaturesWebView);
    } else if (pView == FeaturePropertiesList) {
        mFeatureProperties->showElement(feature);
        mStackedWidget->setCurrentIndex(FeaturePropertiesList);
    }
}

void UBFeaturesCentralWidget::switchTo(StackElement pView)
{
    mStackedWidget->setCurrentIndex(pView);
}

void UBFeaturesCentralWidget::setPropertiesPixmap(const QPixmap &pix)
{
    mFeatureProperties->setOrigPixmap(pix);
}

void UBFeaturesCentralWidget::setPropertiesThumbnail(const QPixmap &pix)
{
    mFeatureProperties->setThumbnail(pix);
}

UBFeature UBFeaturesCentralWidget::getCurElementFromProperties()
{
    return mFeatureProperties->getCurrentElement();    
}

void UBFeaturesCentralWidget::showAdditionalData(AddWidget pWidgetType, AddWidgetState pState)
{
    if (!mAdditionalDataContainer->widget(pWidgetType)) {
        qDebug() << "can't find widget specified by UBFeaturesCentralWidget::showAdditionalData(AddWidget pWidgetType, AddWidgetState pState)";
        return;
    }

    mAdditionalDataContainer->setMaximumHeight(mAdditionalDataContainer->widget(pWidgetType)->sizeHint().height());

    mAdditionalDataContainer->setCurrentIndex(pWidgetType);
    mAdditionalDataContainer->show();
    emit lockMainWidget(pState == Modal ? true : false);
}

void UBFeaturesCentralWidget::setLockedExcludingAdditional(bool pLock)
{
//    Lock all the members excluding mAdditionalDataContainer
    mStackedWidget->setEnabled(!pLock);
}

void UBFeaturesCentralWidget::createNewFolderSlot(QString pStr)
{
    emit createNewFolderSignal(pStr);
    hideAdditionalData();
}

void UBFeaturesCentralWidget::hideAdditionalData()
{
    emit lockMainWidget(false);
    mAdditionalDataContainer->hide();
}

void UBFeaturesCentralWidget::scanStarted()
{
    showAdditionalData(ProgressBarWidget);
}

void UBFeaturesCentralWidget::scanFinished()
{
    hideAdditionalData();
    UBFeaturesController::setRTEIsLoaded(true);
    // UBApplication::mainWindow->actionRichTextEditor->setEnabled(true); ALTI/AOU - 20140606 : RichTextEditor tool isn't available  anymore.
}

UBFeaturesNewFolderDialog::UBFeaturesNewFolderDialog(QWidget *parent) : QWidget(parent)
  , acceptText(tr("Accept"))
  , cancelText(tr("Cancel"))
  , labelText(tr("Enter a new folder name"))
{
    this->setStyleSheet("background:white;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();

    QLabel *mLabel = new QLabel(labelText, this);
    mLineEdit = new QLineEdit(this);

    mValidator = new QRegExpValidator(QRegExp("[^\\/\\:\\?\\*\\|\\<\\>\\\"]{2,}"), this);
    mLineEdit->setValidator(mValidator);
    labelLayout->addWidget(mLabel);
    labelLayout->addWidget(mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton = new QPushButton(acceptText, this);
    QPushButton *cancelButton = new QPushButton(cancelText, this);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    acceptButton->setEnabled(false);

    connect(acceptButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mLineEdit, SIGNAL(textEdited(QString)), this, SLOT(reactOnTextChanged(QString)));

    reactOnTextChanged(QString());
}

void UBFeaturesNewFolderDialog::setRegexp(const QRegExp pRegExp)
{
    mValidator->setRegExp(pRegExp);
}
bool UBFeaturesNewFolderDialog::validString(const QString &pStr)
{
    return mLineEdit->hasAcceptableInput() && !mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive);
}

void UBFeaturesNewFolderDialog::accept()
{
//     Setting all the constraints we need
    emit createNewFolder(mLineEdit->text().trimmed());
    mLineEdit->clear();
}
void UBFeaturesNewFolderDialog::reject()
{
    mLineEdit->clear();
    emit closeDialog();
}
void UBFeaturesNewFolderDialog::setFileNameList(const QStringList &pLst)
{
    mFileNameList = pLst;
}
void UBFeaturesNewFolderDialog::reactOnTextChanged(const QString &pStr)
{
    if (validString(pStr)) {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:white;");
    } else {
        acceptButton->setEnabled(false);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
    }
}

UBFeaturesProgressInfo::UBFeaturesProgressInfo(QWidget *parent) :
    QWidget(parent),
    mProgressBar(0),
    mCommonInfoLabel(0),
    mDetailedInfoLabel(0)
{
    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mProgressBar = new QProgressBar(this);
//    setting defaults
    mProgressBar->setMinimum(0);
    mProgressBar->setMaximum(100000);
    mProgressBar->setValue(0);

    mProgressBar->setStyleSheet("background:white");

    mCommonInfoLabel = new QLabel(this);
    mDetailedInfoLabel = new QLabel(this);
    mDetailedInfoLabel->setAlignment(Qt::AlignRight);
    mCommonInfoLabel->hide();
    mDetailedInfoLabel->hide();

    mainLayer->addWidget(mCommonInfoLabel);
    mainLayer->addWidget(mDetailedInfoLabel);
    mainLayer->addWidget(mProgressBar);
}

void UBFeaturesProgressInfo::setCommmonInfoText(const QString &str)
{
    mProgressBar->setFormat(tr("Loading ") + str + " (%p%)");
}

void UBFeaturesProgressInfo::setDetailedInfoText(const QString &str)
{
    mDetailedInfoLabel->setText(str);
}

void UBFeaturesProgressInfo::setProgressMax(int pValue)
{
    mProgressBar->setMaximum(pValue);
}

void UBFeaturesProgressInfo::setProgressMin(int pValue)
{
    mProgressBar->setMinimum(pValue);
}

void UBFeaturesProgressInfo::increaseProgressValue()
{
    mProgressBar->setValue(mProgressBar->value() + 1);
}

void UBFeaturesProgressInfo::sendFeature(UBFeature pFeature)
{
    Q_UNUSED(pFeature);
}


UBFeaturesWebView::UBFeaturesWebView(QWidget* parent, const char* name):QWidget(parent)
    , mpView(NULL)
    , mpWebSettings(NULL)
    , mpLayout(NULL)
    , mpSankoreAPI(NULL)
{
    setObjectName(name);

    SET_STYLE_SHEET();

    mpLayout = new QVBoxLayout();
    setLayout(mpLayout);

    mpView = new QWebView(this);
    mpView->setObjectName("SearchEngineView");
    mpSankoreAPI = new UBWidgetUniboardAPI(UBApplication::boardController->activeScene());
    mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
    connect(mpView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(javaScriptWindowObjectCleared()));
    mpWebSettings = QWebSettings::globalSettings();
    mpWebSettings->setAttribute(QWebSettings::JavaEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::PluginsEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    mpWebSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);

    mpLayout->addWidget(mpView);
    mpLayout->setMargin(0);

    connect(mpView, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
}

UBFeaturesWebView::~UBFeaturesWebView()
{
    if( NULL != mpSankoreAPI )
    {
        delete mpSankoreAPI;
        mpSankoreAPI = NULL;
    }
    if( NULL != mpView )
    {
        delete mpView;
        mpView = NULL;
    }
    if( NULL != mpLayout )
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}

void UBFeaturesWebView::javaScriptWindowObjectCleared()
{
    mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
}

void UBFeaturesWebView::showElement(const UBFeature &elem)
{
    QString qsWidgetName;
    QString path = elem.getFullPath().toLocalFile();

    QString qsConfigPath = QString("%0/config.xml").arg(path);

    if(QFile::exists(qsConfigPath))
    {
        QFile f(qsConfigPath);
        if(f.open(QIODevice::ReadOnly))
        {
            QDomDocument domDoc;
            domDoc.setContent(QString(f.readAll()));
            QDomElement root = domDoc.documentElement();

            QDomNode node = root.firstChild();
            while(!node.isNull())
            {
                if(node.toElement().tagName() == "content")
                {
                    QDomAttr srcAttr = node.toElement().attributeNode("src");
                    qsWidgetName = srcAttr.value();
                    break;
                }
                node = node.nextSibling();
            }
            f.close();
        }
    }

    mpView->load(QUrl::fromLocalFile(QString("%0/%1").arg(path).arg(qsWidgetName)));
}

void UBFeaturesWebView::onLoadFinished(bool ok)
{
    if(ok && NULL != mpSankoreAPI){
        mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
    }
}


UBFeatureProperties::UBFeatureProperties( QWidget *parent, const char *name ) : QWidget(parent)
    , mpLayout(NULL)
    , mpButtonLayout(NULL)
    , mpAddPageButton(NULL)
    , mpAddToLibButton(NULL)
    , mpSetAsBackgroundButton(NULL)
    , mpSetAsDefaultBackgroundButton(NULL)
    , mpObjInfoLabel(NULL)
    , mpObjInfos(NULL)
    , mpThumbnail(NULL)
    , mpOrigPixmap(NULL)
    , mpElement(NULL)
{
    setObjectName(name);

    // Create the GUI
    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    maxThumbHeight = height() / 4;

    mpThumbnail = new UBDraggableThumbnail(this);

    mpLayout->addWidget(mpThumbnail, 0);

    mpLayout->addWidget(new QLabel(tr("Add"),this));

    mpButtonLayout = new QHBoxLayout();
    mpLayout->addLayout(mpButtonLayout, 0);

    mpAddPageButton = new UBFeatureItemButton();
    mpAddPageButton->setText(tr("Add to page"));
    mpButtonLayout->addWidget(mpAddPageButton);

    mpSetAsBackgroundButton = new UBFeatureItemButton();
    mpSetAsBackgroundButton->setText(tr("Set as background"));
    mpButtonLayout->addWidget(mpSetAsBackgroundButton);

    // Issue 1684 - CFA - 20131120
    mpSetAsDefaultBackgroundButton = new UBFeatureItemButton();
    mpSetAsDefaultBackgroundButton->setText(tr("Set as default background"));
    mpButtonLayout->addWidget(mpSetAsDefaultBackgroundButton);

    mpAddToLibButton = new UBFeatureItemButton();
    mpAddToLibButton->setText(tr("Add to library"));
    mpButtonLayout->addWidget(mpAddToLibButton);

    mpButtonLayout->addStretch(1);

    mpObjInfoLabel = new QLabel(tr("Object informations"));
    mpObjInfoLabel->setStyleSheet(QString("color: #888888; font-size : 18px; font-weight:bold;"));
    mpLayout->addWidget(mpObjInfoLabel, 0);

    mpObjInfos = new QTreeWidget(this);
    mpObjInfos->setColumnCount(2);
    mpObjInfos->header()->hide();
    mpObjInfos->setAlternatingRowColors(true);
    mpObjInfos->setRootIsDecorated(false);
    mpObjInfos->setObjectName("DockPaletteWidgetBox");
    mpObjInfos->setStyleSheet("background:white;");
    mpLayout->addWidget(mpObjInfos, 1);
    mpLayout->setMargin(0);

    connect( mpAddPageButton, SIGNAL(clicked()), this, SLOT(onAddToPage()) );
    // Issue 1684 - CFA - 20131120
    connect( mpSetAsBackgroundButton, SIGNAL( pressed() ), this, SLOT( setAsBackgroundPressed() ) );
    connect( mpSetAsBackgroundButton, SIGNAL( released() ), this, SLOT( setAsBackgroundReleased() ) );
    connect( mpSetAsDefaultBackgroundButton, SIGNAL( pressed() ), this, SLOT( setAsDefaultBackgroundPressed() ) );
    connect( mpSetAsDefaultBackgroundButton, SIGNAL( released() ), this, SLOT( setAsDefaultBackgroundReleased() ) );
    connect( mpAddToLibButton, SIGNAL( clicked() ), this, SLOT(onAddToLib() ) );
}

UBFeatureProperties::~UBFeatureProperties()
{
    if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
    if ( mpElement )
    {
        delete mpElement;
        mpElement = NULL;
    }
    if ( mpThumbnail )
    {
        delete mpThumbnail;
        mpThumbnail = NULL;
    }
    if ( mpButtonLayout )
    {
        delete mpButtonLayout;
        mpButtonLayout = NULL;
    }
    if ( mpAddPageButton )
    {
        delete mpAddPageButton;
        mpAddPageButton = NULL;
    }
    if ( mpSetAsBackgroundButton )
    {
        delete mpSetAsBackgroundButton;
        mpSetAsBackgroundButton = NULL;
    }
    if (mpSetAsDefaultBackgroundButton)
    {
        delete mpSetAsDefaultBackgroundButton;
        mpSetAsDefaultBackgroundButton = NULL;
    }
    if ( mpAddToLibButton )
    {
        delete mpAddToLibButton;
        mpAddToLibButton = NULL;
    }
    if ( mpObjInfoLabel )
    {
        delete mpObjInfoLabel;
        mpObjInfoLabel = NULL;
    }
    if ( mpObjInfos )
    {
        delete mpObjInfos;
        mpObjInfos = NULL;
    }
}

void UBFeatureProperties::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

void UBFeatureProperties::showEvent (QShowEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

UBFeature UBFeatureProperties::getCurrentElement() const
{
    if ( mpElement )
        return *mpElement;

    return UBFeature();
}

void UBFeatureProperties::setOrigPixmap(const QPixmap &pix)
{

    if (mpOrigPixmap)
        delete mpOrigPixmap;

    mpOrigPixmap = new QPixmap(pix);
}

void UBFeatureProperties::setThumbnail(const QPixmap &pix)
{
    mpThumbnail->setPixmap(pix.scaledToWidth(THUMBNAIL_WIDTH));
    adaptSize();
}

void UBFeatureProperties::adaptSize()
{
    if( NULL != mpOrigPixmap )
    {
        if( width() < THUMBNAIL_WIDTH + 40 )
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( width() - 40 ) );
        }
        else
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( THUMBNAIL_WIDTH ) );
        }
    }
}

void UBFeatureProperties::showElement(const UBFeature &elem)
{
    if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
    if ( mpElement )
    {
        delete mpElement;
        mpElement = NULL;
    }
    mpElement = new UBFeature(elem);
    mpOrigPixmap = new QPixmap(QPixmap::fromImage(elem.getThumbnail()));
    mpThumbnail->setPixmap(QPixmap::fromImage(elem.getThumbnail()).scaledToWidth(THUMBNAIL_WIDTH));
    populateMetadata();

    if ( UBApplication::isFromWeb( elem.getFullPath().toString() ) )
    {
        mpAddToLibButton->show();
        // Issue 1684 - CFA - 201312002 : on ne souhaite pas que ces options soient disponibles directement ici
        mpSetAsBackgroundButton->hide();
        mpSetAsDefaultBackgroundButton->hide();

    }
    else
    {
        mpAddToLibButton->hide();
        if (UBFileSystemUtils::mimeTypeFromFileName( elem.getFullPath().toLocalFile() ).contains("image"))
        { // Issue 1684 - CFA - 20131120
            mpSetAsBackgroundButton->show();
            mpSetAsDefaultBackgroundButton->show();
        }
        else
        {
            mpSetAsBackgroundButton->hide();
            mpSetAsDefaultBackgroundButton->hide();
        }
    }
}

void UBFeatureProperties::populateMetadata()
{
    if(NULL != mpObjInfos){
        mpObjInfos->clear();
        QMap<QString, QString> metas = mpElement->getMetadata();
        QList<QString> lKeys = metas.keys();
        QList<QString> lValues = metas.values();

        for(int i=0; i< metas.size(); i++){
            QStringList values;
            values << lKeys.at(i);
            values << lValues.at(i);
            mpItem = new QTreeWidgetItem(values);
            mpObjInfos->addTopLevelItem(mpItem);
        }
        mpObjInfos->resizeColumnToContents(0);
    }
}

void UBFeatureProperties::onAddToPage()
{
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    UBFeaturesWidget* featuresWidget = qobject_cast<UBFeaturesWidget*>( w );
    if (featuresWidget)
        featuresWidget->getFeaturesController()->addItemToPage( *mpElement );
}

void UBFeatureProperties::onAddToLib()
{
    if ( UBApplication::isFromWeb(mpElement->getFullPath().toString() ) )
    {
        sDownloadFileDesc desc;
        desc.isBackground = false;
        desc.modal = false;
        desc.dest = sDownloadFileDesc::library;
        desc.name = mpElement->getMetadata().value("Title", QString());
        qDebug() << desc.name;
        desc.srcUrl = mpElement->getFullPath().toString();
        UBDownloadManager::downloadManager()->addFileToDownload(desc);
    }
}

void UBFeatureProperties::setAsBackgroundPressed()
{
    mSetAsBackgroundButtonPressedTime = QTime::currentTime();

    mPendingSetAsBackgroundButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(setAsBackgroundReleased()));
}

void UBFeatureProperties::setAsBackgroundReleased()
{
    if (mPendingSetAsBackgroundButtonPressed)
    {
        if( mSetAsBackgroundButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {
            UBApplication::boardController->paletteManager()->toggleImageBackgroundPalette(true, false);
        }
        else
        {
            onSetAsBackground();
        }

        mPendingSetAsBackgroundButtonPressed = false;
    }
}

void UBFeatureProperties::setAsDefaultBackgroundPressed()
{
    mSetAsDefaultBackgroundButtonPressedTime = QTime::currentTime();

    mPendingSetAsDefaultBackgroundButtonPressed = true;
    QTimer::singleShot(1000, this, SLOT(setAsDefaultBackgroundReleased()));
}


void UBFeatureProperties::setAsDefaultBackgroundReleased()
{
    if (mPendingSetAsDefaultBackgroundButtonPressed)
    {
        mPendingSetAsDefaultBackgroundButtonPressed = false;
        if( mSetAsDefaultBackgroundButtonPressedTime.msecsTo(QTime::currentTime()) > 900)
        {            
                UBApplication::boardController->paletteManager()->toggleImageBackgroundPalette(true, true);
        }
        else
        {         
                onSetAsDefaultBackground();
        }        
    }

}

void UBFeatureProperties::onSetAsBackground()
{
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    UBFeaturesWidget* featuresWidget = qobject_cast<UBFeaturesWidget*>( w );
    featuresWidget->getFeaturesController()->addItemAsBackground( *mpElement, false );
}

void UBFeatureProperties::onSetAsDefaultBackground()
{
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    UBFeaturesWidget* featuresWidget = qobject_cast<UBFeaturesWidget*>( w );
    featuresWidget->getFeaturesController()->addItemAsDefaultBackground( *mpElement, false);
}



UBFeatureItemButton::UBFeatureItemButton(QWidget *parent, const char *name):QPushButton(parent)
{
    setObjectName(name);
    setStyleSheet(QString("background-color : #DDDDDD; color : #555555; border-radius : 6px; padding : 5px; font-weight : bold; font-size : 12px;"));
}

UBFeatureItemButton::~UBFeatureItemButton()
{
}

QVariant UBFeaturesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return featuresList->at(index.row()).getDisplayName();
    }

    else if (role == Qt::DecorationRole) {
        return QIcon( QPixmap::fromImage(featuresList->at(index.row()).getThumbnail()));

    } else if (role == Qt::UserRole) {
        return featuresList->at(index.row()).getVirtualPath();

    }	else if (role == Qt::UserRole + 1) {
        //return featuresList->at(index.row()).getType();
        UBFeature f = featuresList->at(index.row());
        return QVariant::fromValue( f );
    }

    return QVariant();
}

QMimeData* UBFeaturesModel::mimeData(const QModelIndexList &indexes) const
{
    UBFeaturesMimeData *mimeData = new UBFeaturesMimeData();
    QList <QUrl> urlList;
    QList <UBFeature> featuresList;
    QByteArray typeData;

    foreach (QModelIndex index, indexes) {

        if (index.isValid()) {
            UBFeature element = data(index, Qt::UserRole + 1).value<UBFeature>();
            urlList.push_back( element.getFullPath() );
            QString curPath = element.getFullPath().toLocalFile();
            featuresList.append(element);

            if (!typeData.isNull()) {
                typeData += UBFeaturesController::featureTypeSplitter();
            }
            typeData += QString::number(element.getType()).toAscii();
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setFeatures(featuresList);
    mimeData->setData(mimeSankoreFeatureTypes, typeData);

    return mimeData;
}

bool UBFeaturesModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row)

    const UBFeaturesMimeData *fMimeData = qobject_cast<const UBFeaturesMimeData*>(mimeData);
    UBFeaturesController *curController = qobject_cast<UBFeaturesController *>(QObject::parent());

    bool dataFromSameModel = false;

    if (fMimeData)
        dataFromSameModel = true;

    if ((!mimeData->hasUrls() && !mimeData->hasImage()) )
        return false;
    if ( action == Qt::IgnoreAction )
        return true;
    if ( column > 0 )
        return false;

    UBFeature parentFeature;
    if (!parent.isValid()) {
        parentFeature = curController->getCurrentElement();
    } else {
        parentFeature = parent.data( Qt::UserRole + 1).value<UBFeature>();
    }

    if (dataFromSameModel) {
        QList<UBFeature> featList = fMimeData->features();
        for (int i = 0; i < featList.count(); i++) {
            UBFeature sourceElement;
            if (dataFromSameModel) {
                sourceElement = featList.at(i);
                moveData(sourceElement, parentFeature, Qt::MoveAction);
            }
        }
    } else if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        foreach (QUrl curUrl, urlList) {
            qDebug() << "URl catched is " << curUrl.toLocalFile();
            curController->moveExternalData(curUrl, parentFeature);
        }
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>( mimeData->imageData() );
        curController->importImage( image, parentFeature );

    }

    return true;
}

void UBFeaturesModel::addItem( const UBFeature &item )
{
    beginInsertRows( QModelIndex(), featuresList->size(), featuresList->size() );
    featuresList->append( item );
    endInsertRows();
}

void UBFeaturesModel::deleteFavoriteItem( const QString &path )
{
    for ( int i = 0; i < featuresList->size(); ++i )
    {
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) &&
            !QString::compare( featuresList->at(i).getVirtualPath(), "/root/favorites", Qt::CaseInsensitive ) )
        {
            removeRow( i, QModelIndex() );
            return;
        }
    }
}

void UBFeaturesModel::deleteItem( const QString &path )
{
    for ( int i = 0; i < featuresList->size(); ++i )
    {
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) )
        {
            removeRow( i, QModelIndex() );
            return;
        }
    }
}

void UBFeaturesModel::deleteItem(const UBFeature &feature)
{
    int i = featuresList->indexOf(feature);
    if (i == -1) {
        qDebug() << "no matches in deleting item from UBFEaturesModel";
        return;
    }
    removeRow(i, QModelIndex());
}

bool UBFeaturesModel::removeRows( int row, int count, const QModelIndex & parent )
{
    if ( row < 0 )
        return false;
    if ( row + count > featuresList->size() )
        return false;
    beginRemoveRows( parent, row, row + count - 1 );
    //featuresList->remove( row, count );
    featuresList->erase( featuresList->begin() + row, featuresList->begin() + row + count );
    endRemoveRows();
    return true;
}

bool UBFeaturesModel::removeRow(  int row, const QModelIndex & parent )
{
    if ( row < 0 )
        return false;
    if ( row >= featuresList->size() )
        return false;
    beginRemoveRows( parent, row, row );
    //featuresList->remove( row );
    featuresList->erase( featuresList->begin() + row );
    endRemoveRows();
    return true;
}

void UBFeaturesModel::moveData(const UBFeature &source, const UBFeature &destination
                               , Qt::DropAction action = Qt::CopyAction, bool deleteManualy)
{
    UBFeaturesController *curController = qobject_cast<UBFeaturesController *>(QObject::parent());
    if (!curController)
        return;

    QString sourcePath = source.getFullPath().toLocalFile();
    QString sourceVirtualPath = source.getVirtualPath();

    UBFeatureElementType sourceType = source.getType();
    QImage sourceIcon = source.getThumbnail();

    Q_ASSERT( QFileInfo( sourcePath ).exists() );

    QString name = QFileInfo( sourcePath ).fileName();
    QString destPath = destination.getFullPath().toLocalFile();

    QString destVirtualPath = destination.getFullVirtualPath();
    QString destFullPath = destPath + "/" + name;

    if ( sourcePath.compare(destFullPath, Qt::CaseInsensitive ) || destination.getType() != FEATURE_TRASH)
    {
        RegisteryEntry r = curController->getRegisteryEntry(source.getName());

        //if the destionation isn't the trash, use the old real path
        //beacause here, desFullPath is equal to the path of the feature of destination.
        //When the source has to be restore in user's file, this is wrong to use the path
        //of the destination feature.
        //issue 1474 - NNE - 20131213
        if(destination.getType() != FEATURE_TRASH && r.filename != ""){
            destFullPath = r.originalRealPath + "/" + r.filename;
        }
        //issue 1474 - NNE - 20131213 : END

        UBFileSystemUtils::copy(sourcePath, destFullPath);

        if (action == Qt::MoveAction) {
            curController->deleteItem( source.getFullPath() );
        }
    }

    //If it's a folder, we have to update the path of each file/folder in the source folder
    if (sourceType == FEATURE_FOLDER) {
        for (int i = 0; i < featuresList->count(); i++) {

            UBFeature &curFeature = (*featuresList)[i];

            QString curFeatureFullPath = curFeature.getFullPath().toLocalFile();
            QString curFeatureVirtualPath = curFeature.getVirtualPath();


            if (UBFileSystemUtils::isPathMatch(curFeatureFullPath, sourcePath) && curFeatureFullPath != sourcePath) {

                UBFeature copyFeature = curFeature;
                QUrl newPath = QUrl::fromLocalFile(curFeatureFullPath.replace(sourcePath, destFullPath));

                QString newVirtualPath = curFeatureVirtualPath.replace(sourceVirtualPath, destVirtualPath);

                if (destination.getType() != FEATURE_TRASH) {
                    // processing copy or move action for real FS
                    if (action == Qt::CopyAction) {
                        copyFeature.setFullPath(newPath);
                    } else {
                        curFeature.setFullPath(newPath);
                    }
                } else {
                    if (action == Qt::CopyAction) {
                        copyFeature.setPermissions(curFeature.getPermissions());
                    } else {
                        curFeature.setPermissions(UBFeature::WRITE_P | UBFeature::DELETE_P);
                        //issue 1474 - NNE - 20131125
                        curFeature.setFullPath(newPath);
                    }
                }
                // processing copy or move action for virtual FS
                if (action == Qt::CopyAction) {
                    copyFeature.setFullVirtualPath(newVirtualPath);
                } else {
                    curFeature.setFullVirtualPath(newVirtualPath);
                }

                if (action == Qt::CopyAction) {
                    addItem(copyFeature);
                }
            }
        }
    }

    UBFeature newElement( destVirtualPath + "/" + name, sourceIcon, name, QUrl::fromLocalFile(destFullPath), sourceType );
    addItem(newElement);
    if (deleteManualy) {
        deleteItem(source);
    }

// Commented because of crashes on mac. But works fine. It is not predictable behavior.
// Please uncomment it if model will not refreshes
//   emit dataRestructured();
}

Qt::ItemFlags UBFeaturesModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
    if ( index.isValid() )
    {
        UBFeature item = index.data( Qt::UserRole + 1 ).value<UBFeature>();
        if ( item.getType() == FEATURE_INTERACTIVE
             || item.getType() == FEATURE_ITEM
             || item.getType() == FEATURE_AUDIO
             || item.getType() == FEATURE_VIDEO
             || item.getType() == FEATURE_IMAGE
             || item.getType() == FEATURE_FLASH
             || item.getType() == FEATURE_INTERNAL
             || item.getType() == FEATURE_LINK
             || item.getType() == FEATURE_BOOKMARK
             || item.getType() == FEATURE_FOLDER)

            resultFlags |= Qt::ItemIsDragEnabled;

        if ( item.isFolder() && !item.getVirtualPath().isNull() )
            resultFlags |= Qt::ItemIsDropEnabled;
    }

    return resultFlags;
}


QStringList UBFeaturesModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

int UBFeaturesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !featuresList)
        return 0;
    else
        return featuresList->size();
}

void UBFeaturesModel::rename(UBFeature &feature, const QString newName) const
{
    QString path = feature.getFullPath().toLocalFile();

    UBFileSystemUtils::rename(path, newName);
    feature.setName(newName);
}

bool UBFeaturesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    UBFeature f = index.data(Qt::UserRole + 1).value<UBFeature>();
    if (f.getType() == FEATURE_RTE)
        return false;

    QString path = index.data( Qt::UserRole ).toString();

    return filterRegExp().exactMatch(path);
}

bool UBFeaturesProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    UBFeature leftFeature = left.data(Qt::UserRole + 1).value<UBFeature>();
    UBFeature rightFeature = right.data(Qt::UserRole + 1).value<UBFeature>();

    if (leftFeature.getType() == FEATURE_FOLDER) {
        if (rightFeature.getType() == FEATURE_FOLDER) {
            return leftFeature.getSortKey() < rightFeature.getSortKey();
        } else {
            return true;
        }
    } else {
        if (rightFeature.getType() == FEATURE_FOLDER) {
            return false;
        } else {
            return leftFeature.getSortKey() < rightFeature.getSortKey();
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

bool UBFeaturesSearchProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    /*QString name = sourceModel()->data(index, Qt::DisplayRole).toString();
    eUBLibElementType type = (eUBLibElementType)sourceModel()->data(index, Qt::UserRole + 1).toInt();*/

    UBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<UBFeature>();
    bool isFile = feature.getType() == FEATURE_INTERACTIVE
            || feature.getType() == FEATURE_INTERNAL
            || feature.getType() == FEATURE_ITEM
            || feature.getType() == FEATURE_AUDIO
            || feature.getType() == FEATURE_VIDEO
            || feature.getType() == FEATURE_IMAGE;

    return isFile
            && feature.getFullVirtualPath().contains(mFilterPrefix)
            && filterRegExp().exactMatch( feature.getName() );
}

bool UBFeaturesPathProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    UBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<UBFeature>();

    //issue 1474 - NNE - 20131122
    return feature.isFolder() && UBFileSystemUtils::isPathMatch(path, feature.getFullVirtualPath());
}

QString	UBFeaturesItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(locale)

    QString text = value.toString();
    text = text.replace(".wgt", "");
    text = text.replace(".wgs", "");
    text = text.replace(".swf","");
    if (listView)
    {
        const QFontMetrics fm = listView->fontMetrics();
        const QSize iSize = listView->gridSize();
        return elidedText( fm, iSize.width(), Qt::ElideRight, text );
    }
    return text;
}

UBFeaturesPathItemDelegate::UBFeaturesPathItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
    arrowPixmap = new QPixmap(":images/navig_arrow.png");
}

QString	UBFeaturesPathItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)

    return QString();
}

void UBFeaturesPathItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    UBFeature feature = index.data( Qt::UserRole + 1 ).value<UBFeature>();
    QRect rect = option.rect;
    if ( !feature.getFullPath().isEmpty() )
    {
        painter->drawPixmap( rect.left() - 10, rect.center().y() - 5, *arrowPixmap );
    }
    painter->drawImage( rect.left() + 5, rect.center().y() - 5, feature.getThumbnail().scaledToHeight( 30, Qt::SmoothTransformation ) );
}

UBFeaturesPathItemDelegate::~UBFeaturesPathItemDelegate()
{
    if ( arrowPixmap )
    {
        delete arrowPixmap;
        arrowPixmap = NULL;
    }
}
