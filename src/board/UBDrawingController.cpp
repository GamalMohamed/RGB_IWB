#include "UBDrawingController.h"

#include "core/UBSettings.h"
#include "core/UBApplication.h"

#include "domain/UBGraphicsScene.h"
#include "board/UBBoardController.h"
#include "board/UBBoardPaletteManager.h"
#include "domain/UBEditableGraphicsPolygonItem.h"

#include "board/UBBoardView.h"

#include "gui/UBMainWindow.h"
#include "core/memcheck.h"

UBDrawingController* UBDrawingController::sDrawingController = 0;


UBDrawingController* UBDrawingController::drawingController()
{
    if(!sDrawingController)
        sDrawingController = new UBDrawingController();

    return sDrawingController;
}

void UBDrawingController::destroy()
{
    if(sDrawingController)
        delete sDrawingController;
    sDrawingController = NULL;
}

UBDrawingController::UBDrawingController(QObject * parent)
    : QObject(parent)
    , mActiveRuler(NULL)
    , mStylusTool((UBStylusTool::Enum)-1)
    , mLatestDrawingTool((UBStylusTool::Enum)-1)
	, mIsDesktopMode(false)
{
    connect(UBSettings::settings(), SIGNAL(colorContextChanged()),
            this, SIGNAL(colorPaletteChanged()));
    connect(UBApplication::mainWindow->actionPen, SIGNAL(triggered(bool)),
            this, SLOT(penToolSelected(bool)));
    connect(UBApplication::mainWindow->actionEraser, SIGNAL(triggered(bool)),
            this, SLOT(eraserToolSelected(bool)));
    connect(UBApplication::mainWindow->actionMarker, SIGNAL(triggered(bool)),
            this, SLOT(markerToolSelected(bool)));
    connect(UBApplication::mainWindow->actionSelector, SIGNAL(triggered(bool)),
            this, SLOT(selectorToolSelected(bool)));
    connect(UBApplication::mainWindow->actionPlay, SIGNAL(triggered(bool)),
            this, SLOT(playToolSelected(bool)));
    connect(UBApplication::mainWindow->actionHand, SIGNAL(triggered(bool)),
            this, SLOT(handToolSelected(bool)));
    connect(UBApplication::mainWindow->actionZoomIn, SIGNAL(triggered(bool)),
            this, SLOT(zoomInToolSelected(bool)));
    connect(UBApplication::mainWindow->actionZoomOut, SIGNAL(triggered(bool)),
            this, SLOT(zoomOutToolSelected(bool)));
    connect(UBApplication::mainWindow->actionPointer, SIGNAL(triggered(bool)),
            this, SLOT(pointerToolSelected(bool)));
    connect(UBApplication::mainWindow->actionLine, SIGNAL(triggered(bool)),
            this, SLOT(lineToolSelected(bool)));
    connect(UBApplication::mainWindow->actionText, SIGNAL(triggered(bool)),
            this, SLOT(textToolSelected(bool)));
    connect(UBApplication::mainWindow->actionCapture, SIGNAL(triggered(bool)),
            this, SLOT(captureToolSelected(bool)));
    connect(UBApplication::boardController, SIGNAL(activeSceneChanged()),
            this, SLOT(onActiveSceneChanged())); //EV-7 - NNE - 20140210 : Maybe is no the right place to do this...


    //Removed--connect(UBApplication::mainWindow->actionRichTextEditor, SIGNAL(triggered(bool)), this, SLOT(richTextToolSelected(bool))); ALTI/AOU - 20140606 : RichTextEditor tool isn't available anymore.
}

UBDrawingController::~UBDrawingController()
{
    // NOOP
}

int UBDrawingController::stylusTool()
{
    return mStylusTool;
}


int UBDrawingController::latestDrawingTool()
{
    return mLatestDrawingTool;
}

void UBDrawingController::setStylusTool(int tool)
{
    if (tool != mStylusTool)
    {
        if(tool != UBStylusTool::Drawing)
        {
            UBApplication::boardController->activeScene()->deselectAllItems();

            //Issue retours 2.4RC1 - CFA - 20140217 : hide drawing Palette children on setting stylus tool
            UBApplication::boardController->paletteManager()->drawingPalette()->hideSubPalettes();
        }
        else
        {
            foreach(QGraphicsItem *gi, UBApplication::boardController->activeScene()->selectedItems())
            {
                UBShapeFactory::desactivateEditionMode(gi);
            }
        }        

        if (mStylusTool == UBStylusTool::Pen || mStylusTool == UBStylusTool::Marker
                || mStylusTool == UBStylusTool::Line)
        {
            mLatestDrawingTool = mStylusTool;
        }

        if (tool == UBStylusTool::Pen || tool == UBStylusTool::Line)
        {
             emit lineWidthIndexChanged(UBSettings::settings()->penWidthIndex());
             emit colorIndexChanged(UBSettings::settings()->penColorIndex());
        }
        else if (tool == UBStylusTool::Marker)
        {
            emit lineWidthIndexChanged(UBSettings::settings()->markerWidthIndex());
            emit colorIndexChanged(UBSettings::settings()->markerColorIndex());
        }

        mStylusTool = (UBStylusTool::Enum)tool;


        if (mStylusTool == UBStylusTool::Pen)
            UBApplication::mainWindow->actionPen->setChecked(true);
        else if (mStylusTool == UBStylusTool::Eraser)
            UBApplication::mainWindow->actionEraser->setChecked(true);
        else if (mStylusTool == UBStylusTool::Marker)
            UBApplication::mainWindow->actionMarker->setChecked(true);
        else if (mStylusTool == UBStylusTool::Selector)
            UBApplication::mainWindow->actionSelector->setChecked(true);
        else if (mStylusTool == UBStylusTool::Play)
            UBApplication::mainWindow->actionPlay->setChecked(true);
        else if (mStylusTool == UBStylusTool::Hand)
            UBApplication::mainWindow->actionHand->setChecked(true);
        else if (mStylusTool == UBStylusTool::ZoomIn)
            UBApplication::mainWindow->actionZoomIn->setChecked(true);
        else if (mStylusTool == UBStylusTool::ZoomOut)
            UBApplication::mainWindow->actionZoomOut->setChecked(true);
        else if (mStylusTool == UBStylusTool::Pointer)
            UBApplication::mainWindow->actionPointer->setChecked(true);
        else if (mStylusTool == UBStylusTool::Line)
            UBApplication::mainWindow->actionLine->setChecked(true);
        else if (mStylusTool == UBStylusTool::Text)
            UBApplication::mainWindow->actionText->setChecked(true);
        /* ALTI/AOU - 20140606 : RichTextEditor tool isn't available anymore.
        else if (mStylusTool == UBStylusTool::RichText)
            UBApplication::mainWindow->actionRichTextEditor->setChecked(true);
        */
        else if (mStylusTool == UBStylusTool::Capture)
            UBApplication::mainWindow->actionCapture->setChecked(true);

        if(mStylusTool != UBStylusTool::Drawing){
            UBApplication::boardController->shapeFactory().desactivate();
        }

        emit stylusToolChanged(tool);
        emit colorPaletteChanged();
    }

    //EV-7 : ce n'est pas de la responsabilité de cette méthode de le faire ... mais plus beaucoup de temps..
    deactivateCreationModeForGraphicsPathItems();
}

void UBDrawingController::onActiveSceneChanged()
{
    foreach (QGraphicsItem* gi, UBApplication::boardController->activeScene()->items())
    {
        if (gi->type() == UBGraphicsItemType::GraphicsPathItemType)
        {
            UBEditableGraphicsPolygonItem* path = dynamic_cast<UBEditableGraphicsPolygonItem*>(gi);
            if (path){
                path->setIsInCreationMode(false);
            }
        }
    }

    UBApplication::boardController->shapeFactory().terminateShape();
}

void UBDrawingController::deactivateCreationModeForGraphicsPathItems()
{
    foreach (QGraphicsItem* gi, UBApplication::boardController->activeScene()->items())
    {
        if (gi->type() == UBGraphicsItemType::GraphicsPathItemType)
        {
            UBEditableGraphicsPolygonItem* path = dynamic_cast<UBEditableGraphicsPolygonItem*>(gi);
            if (path){
                path->setIsInCreationMode(false);
                UBApplication::boardController->shapeFactory().desactivate();
                if (path->path().elementCount() < 2)
                    UBApplication::boardController->controlView()->scene()->removeItem(gi);
            }
        }
    }
    UBApplication::boardController->controlView()->resetCachedContent();
}

bool UBDrawingController::isDrawingTool()
{
    return (stylusTool() == UBStylusTool::Pen)
            || (stylusTool() == UBStylusTool::Marker)
            || (stylusTool() == UBStylusTool::Line);
}


int UBDrawingController::currentToolWidthIndex()
{
    if (stylusTool() == UBStylusTool::Pen || stylusTool() == UBStylusTool::Line)
    {
        return UBSettings::settings()->penWidthIndex();
    }
    else if (stylusTool() == UBStylusTool::Marker)
    {
        return UBSettings::settings()->markerWidthIndex();
    }
    else
    {
        return -1;
    }
}

qreal UBDrawingController::currentToolWidth()
{
    if (stylusTool() == UBStylusTool::Pen || stylusTool() == UBStylusTool::Line)
    {
        return UBSettings::settings()->currentPenWidth();
    }
    else if (stylusTool() == UBStylusTool::Marker)
    {
        return UBSettings::settings()->currentMarkerWidth();
    }
    else
    {
        //failsafe
        return UBSettings::settings()->currentPenWidth();
    }
}

void UBDrawingController::setLineWidthIndex(int index)
{
    if (stylusTool() == UBStylusTool::Marker)
    {
        UBSettings::settings()->setMarkerWidthIndex(index);
    }
    else
    {
        UBSettings::settings()->setPenWidthIndex(index);

        if(stylusTool() != UBStylusTool::Line
            && stylusTool() != UBStylusTool::Selector)
        {
            setStylusTool(UBStylusTool::Pen);
        }
    }

    emit lineWidthIndexChanged(index);
}

int UBDrawingController::currentToolColorIndex()
{
    if (stylusTool() == UBStylusTool::Pen || stylusTool() == UBStylusTool::Line)
    {
        return UBSettings::settings()->penColorIndex();
    }
    else if (stylusTool() == UBStylusTool::Marker)
    {
        return UBSettings::settings()->markerColorIndex();
    }
    else
    {
        return -1;
    }
}

QColor UBDrawingController::currentToolColor()
{
    return toolColor(UBSettings::settings()->isDarkBackground());
}


QColor UBDrawingController::toolColor(bool onDarkBackground)
{
    if (stylusTool() == UBStylusTool::Pen || stylusTool() == UBStylusTool::Line)
    {
        return UBSettings::settings()->penColor(onDarkBackground);
    }
    else if (stylusTool() == UBStylusTool::Marker)
    {
        return UBSettings::settings()->markerColor(onDarkBackground);
    }
    else
    {
        //failsafe
        if (onDarkBackground)
        {
            return Qt::white;
        }
        else
        {
            return Qt::black;
        }
    }
}

void UBDrawingController::setColorIndex(int index)
{
    Q_ASSERT(index >= 0 && index < UBSettings::settings()->colorPaletteSize);

    if (stylusTool() == UBStylusTool::Marker)
    {
        UBSettings::settings()->setMarkerColorIndex(index);
    }
    else
    {
        UBSettings::settings()->setPenColorIndex(index);
    }

    emit colorIndexChanged(index);
}

void UBDrawingController::setEraserWidthIndex(int index)
{
    setStylusTool(UBStylusTool::Eraser);
    UBSettings::settings()->setEraserWidthIndex(index);
}

void UBDrawingController::setPenColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        UBSettings::settings()->boardPenDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        UBSettings::settings()->boardPenLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void UBDrawingController::setMarkerColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        UBSettings::settings()->boardMarkerDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        UBSettings::settings()->boardMarkerLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void UBDrawingController::setMarkerAlpha(qreal alpha)
{
    UBSettings::settings()->boardMarkerLightBackgroundColors->setAlpha(alpha);
    UBSettings::settings()->boardMarkerLightBackgroundSelectedColors->setAlpha(alpha);

    UBSettings::settings()->boardMarkerDarkBackgroundColors->setAlpha(alpha);
    UBSettings::settings()->boardMarkerDarkBackgroundSelectedColors->setAlpha(alpha);

    UBSettings::settings()->boardMarkerAlpha->set(alpha);

    emit colorPaletteChanged();
}


void UBDrawingController::penToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Pen);
        emit StylusSelected();
    }
}

void UBDrawingController::eraserToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Eraser);
        emit StylusSelected();
    }
}

void UBDrawingController::markerToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Marker);
        emit StylusSelected();
    }
}

void UBDrawingController::lineToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Line);
        emit StylusSelected();
    }
}


void UBDrawingController::playToolSelected(bool checked)
{
    //Interact with objects
    if (checked)
    {
        setStylusTool(UBStylusTool::Play);
        emit StylusUnSelected();
    }
}

void UBDrawingController::textToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Text);
        emit StylusUnSelected();
    }
}

void UBDrawingController::handToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Hand);
        emit StylusUnSelected();
    }
}

void UBDrawingController::pointerToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Pointer);
        emit StylusUnSelected();
    }
}

void UBDrawingController::zoomInToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::ZoomIn);
        emit StylusUnSelected();
    }
}

void UBDrawingController::zoomOutToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::ZoomOut);
        emit StylusUnSelected();
    }
}

void UBDrawingController::selectorToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Selector);
        emit StylusUnSelected();
    }
}

void UBDrawingController::captureToolSelected(bool checked)
{
    if (checked)
    {
        setStylusTool(UBStylusTool::Capture);
        emit StylusUnSelected();
    }
}

//Removed!
void UBDrawingController::richTextToolSelected(bool checked)
{
    if (checked)
        setStylusTool(UBStylusTool::RichText);
}
