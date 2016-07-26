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



#include "UBBoardView.h"

#include <QtGui>
#include <QtXml>
#include <QListView>

#include "UBDrawingController.h"

#include "frameworks/UBGeometryUtils.h"
#include "frameworks/UBPlatformUtils.h"

#include "core/UBSettings.h"
#include "core/UBMimeData.h"
#include "core/UBApplication.h"
#include "core/UBSetting.h"
#include "core/UBPersistenceManager.h"
#include "core/UB.h"

#include "network/UBHttpGet.h"

#include "gui/UBStylusPalette.h"
#include "gui/UBRubberBand.h"
#include "gui/UBToolWidget.h"
#include "gui/UBResources.h"
#include "gui/UBMainWindow.h"
#include "gui/UBThumbnailWidget.h"
#include "gui/UBTeacherGuideWidgetsTools.h"

#include "board/UBBoardController.h"
#include "board/UBBoardPaletteManager.h"

#ifdef Q_WS_MAC
#include "desktop/UBDesktopAnnotationController.h"
#include "desktop/UBDesktopPalette.h"
#endif

#include "domain/UBGraphicsTextItem.h"
#include "domain/UBGraphicsPixmapItem.h"
#include "domain/UBGraphicsWidgetItem.h"
#include "domain/UBGraphicsPDFItem.h"
#include "domain/UBGraphicsPolygonItem.h"
#include "domain/UBItem.h"
#include "domain/UBGraphicsMediaItem.h"
#include "domain/UBGraphicsSvgItem.h"
#include "domain/UBGraphicsGroupContainerItem.h"
#include "domain/UBGraphicsStrokesGroup.h"
#include "domain/UBGraphicsEllipseItem.h"
#include "domain/UBGraphicsProxyWidget.h"

#include "document/UBDocumentProxy.h"

#include "tools/UBGraphicsRuler.h"
#include "tools/UBGraphicsCurtainItem.h"
#include "tools/UBGraphicsCompass.h"
#include "tools/UBGraphicsCache.h"
#include "tools/UBGraphicsTriangle.h"
#include "tools/UBGraphicsProtractor.h"
#include "tools/UBGraphicsAristo.h"

#include "customWidgets/UBGraphicsItemAction.h"

#include "core/memcheck.h"

#include "domain/UBShapeFactory.h"

UBBoardView::UBBoardView (UBBoardController* pController, QWidget* pParent, bool isControl, bool isDesktop)
: QGraphicsView (pParent)
, mController (pController)
, mIsCreatingTextZone (false)
, mIsCreatingSceneGrabZone (false)
, mOkOnWidget(false)
, suspendedMousePressEvent(NULL)
, mLongPressInterval(1000)
, mIsDragInProgress(false)
, mMultipleSelectionIsEnabled(false)
, bIsControl(isControl)
, bIsDesktop(isDesktop)
, mRubberBandInPlayMode(false) //enables rubberband with play tool
{
  init ();

  mFilterZIndex = false;

  mLongPressTimer.setInterval(mLongPressInterval);
  mLongPressTimer.setSingleShot(true);
}

UBBoardView::UBBoardView (UBBoardController* pController, int pStartLayer, int pEndLayer, QWidget* pParent, bool isControl, bool isDesktop)
: QGraphicsView (pParent)
, mController (pController)
, suspendedMousePressEvent(NULL)
, mLongPressInterval(1000)
, mIsDragInProgress(false)
, mMultipleSelectionIsEnabled(false)
, bIsControl(isControl)
, bIsDesktop(isDesktop)
{
  init ();

  mStartLayer = pStartLayer;
  mEndLayer = pEndLayer;

  mFilterZIndex = true;

  mLongPressTimer.setInterval(mLongPressInterval);
  mLongPressTimer.setSingleShot(true);
}

UBBoardView::~UBBoardView () {
  //NOOP
    if (suspendedMousePressEvent)
        delete suspendedMousePressEvent;
}

void UBBoardView::init ()
{
  connect (UBSettings::settings ()->boardPenPressureSensitive, SIGNAL (changed (QVariant)),
           this, SLOT (settingChanged (QVariant)));

  connect (UBSettings::settings ()->boardMarkerPressureSensitive, SIGNAL (changed (QVariant)),
           this, SLOT (settingChanged (QVariant)));

  connect (UBSettings::settings ()->boardUseHighResTabletEvent, SIGNAL (changed (QVariant)),
           this, SLOT (settingChanged (QVariant)));

  setWindowFlags (Qt::FramelessWindowHint);
  setFrameStyle (QFrame::NoFrame);
  setRenderHints (QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
  setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
  setAcceptDrops (true);

  setOptimizationFlag (QGraphicsView::IndirectPainting); // enable UBBoardView::drawItems filter

  mTabletStylusIsPressed = false;
  mMouseButtonIsPressed = false;
  mPendingStylusReleaseEvent = false;

  setCacheMode (QGraphicsView::CacheBackground);

  mUsingTabletEraser = false;
  mIsCreatingTextZone = false;
  mRubberBand = 0;
  mUBRubberBand = 0;

  mVirtualKeyboardActive = false;

  settingChanged (QVariant ());

  unsetCursor();

  movingItem = NULL;
  mWidgetMoved = false;
}

UBGraphicsScene*
UBBoardView::scene ()
{
  return qobject_cast<UBGraphicsScene*> (QGraphicsView::scene ());
}

void
UBBoardView::hideEvent (QHideEvent * event)
{
  Q_UNUSED (event);
  emit hidden ();
}

void
UBBoardView::showEvent (QShowEvent * event)
{
  Q_UNUSED (event);
  emit shown ();
}

void
UBBoardView::keyPressEvent (QKeyEvent *event)
{
  // send to the scene anyway
  QApplication::sendEvent (scene (), event);

  if (!event->isAccepted ())
    {
      switch (event->key ())
        {
        case Qt::Key_Up:
        case Qt::Key_PageUp:
        case Qt::Key_Left:
          {
            mController->previousScene ();
            break;
          }

        case Qt::Key_Down:
        case Qt::Key_PageDown:
        case Qt::Key_Right:
        case Qt::Key_Space:
          {
            mController->nextScene ();
            break;
          }

        case Qt::Key_Home:
          {
            mController->firstScene ();
            break;
          }
        case Qt::Key_End:
          {
            mController->lastScene ();
            break;
          }
        case Qt::Key_Insert:
          {
            mController->addScene ();
            break;
          }
        case Qt::Key_Control:
        case Qt::Key_Shift:
          {
            setMultiselection(true);
          }break;
        }


      qDebug() << event->modifiers ();

      if (event->modifiers () & Qt::ControlModifier) // keep only ctrl/cmd keys
        {
          switch (event->key ())
            {
            case Qt::Key_Plus:
            case Qt::Key_I:
              {
                mController->zoomIn ();
                event->accept ();
                break;
              }
            case Qt::Key_Minus:
            case Qt::Key_O:
              {
                mController->zoomOut ();
                event->accept ();
                break;
              }
            case Qt::Key_0:
              {
                mController->zoomRestore ();
                event->accept ();
                break;
              }
            case Qt::Key_Left:
              {
                mController->handScroll (-100, 0);
                event->accept ();
                break;
              }
            case Qt::Key_Right:
              {
                mController->handScroll (100, 0);
                event->accept ();
                break;
              }
            case Qt::Key_Up:
              {
                mController->handScroll (0, -100);
                event->accept ();
                break;
              }
            case Qt::Key_Down:
              {
                mController->handScroll (0, 100);
                event->accept ();
                break;
              }
            default:
              {
                // NOOP
              }
            }
        }
    }

    // if ctrl of shift was pressed combined with other keys - we need to disable multiple selection.
    if (event->isAccepted())
        setMultiselection(false);
}


void UBBoardView::keyReleaseEvent(QKeyEvent *event)
{
   // if (!event->isAccepted ())
    {
        if (Qt::Key_Shift == event->key()
          ||Qt::Key_Control == event->key())
        {
            setMultiselection(false);
        }
    }

    QGraphicsView::keyReleaseEvent(event);
}

bool
UBBoardView::event (QEvent * e)
{
  if (e->type () == QEvent::Gesture)
    {
      QGestureEvent *gestureEvent = dynamic_cast<QGestureEvent *> (e);
      if (gestureEvent)
        {
          QSwipeGesture* swipe = dynamic_cast<QSwipeGesture*> (gestureEvent->gesture (Qt::SwipeGesture));
          if (swipe)
            {
              if (swipe->horizontalDirection () == QSwipeGesture::Left)
                {
                  mController->previousScene ();
                  gestureEvent->setAccepted (swipe, true);
                }

              if (swipe->horizontalDirection () == QSwipeGesture::Right)
                {
                  mController->nextScene ();
                  gestureEvent->setAccepted (swipe, true);
                }
            }
        }
    }

  return QGraphicsView::event (e);
}

void UBBoardView::tabletEvent (QTabletEvent * event)
{
    if (!mUseHighResTabletEvent) {
        event->setAccepted (false);
        return;
    }

    UBDrawingController *dc = UBDrawingController::drawingController ();

    QPointF tabletPos = event->pos();
    UBStylusTool::Enum currentTool = (UBStylusTool::Enum)dc->stylusTool ();

    if (event->type () == QEvent::TabletPress || event->type () == QEvent::TabletEnterProximity) {
        if (event->pointerType () == QTabletEvent::Eraser) {
            dc->setStylusTool (UBStylusTool::Eraser);
            mUsingTabletEraser = true;
        }
        else {
            if (mUsingTabletEraser && currentTool == UBStylusTool::Eraser)
                dc->setStylusTool (dc->latestDrawingTool ());

            mUsingTabletEraser = false;
        }
    }

    // if event are not Pen events, we drop the tablet stuff and route everything through mouse event
    if (currentTool != UBStylusTool::Pen && currentTool != UBStylusTool::Line && currentTool != UBStylusTool::Marker && !mMarkerPressureSensitive){
        event->setAccepted (false);
        return;
    }

    QPointF scenePos = viewportTransform ().inverted ().map (tabletPos);

    qreal pressure = 1.0;
    if (((currentTool == UBStylusTool::Pen || currentTool == UBStylusTool::Line) && mPenPressureSensitive) || (currentTool == UBStylusTool::Marker && mMarkerPressureSensitive))
        pressure = event->pressure ();


    bool acceptEvent = true;

#ifdef Q_WS_MAC
    //Work around #1388. After selecting annotation tool in desktop mode, annotation view appears on top when
    //using Mac OS X. In this case tablet event should send mouse event so as to let user interact with
    //stylus palette.
    Q_ASSERT(UBApplication::applicationController->uninotesController());
    if (UBApplication::applicationController->uninotesController()->drawingView() == this) {
        if (UBApplication::applicationController->uninotesController()->desktopPalettePath().contains(event->pos())) {
            acceptEvent = false;
        }
    }
#endif

    switch (event->type ()) {
    case QEvent::TabletPress: {
        mTabletStylusIsPressed = true;
        scene()->inputDevicePress (scenePos, pressure);

        break;
    }
    case QEvent::TabletMove: {
        if (mTabletStylusIsPressed)
            scene ()->inputDeviceMove (scenePos, pressure);

        acceptEvent = false; // rerouted to mouse move

        break;

    }
    case QEvent::TabletRelease: {
        UBStylusTool::Enum currentTool = (UBStylusTool::Enum)dc->stylusTool ();
        scene ()->setToolCursor (currentTool);
        setToolCursor (currentTool);

        scene ()->inputDeviceRelease ();

        mPendingStylusReleaseEvent = false;

        mTabletStylusIsPressed = false;
        mMouseButtonIsPressed = false;

        break;
    }
    default: {
        //NOOP - avoid compiler warning
    }
    }

    // ignore mouse press and mouse move tablet event so that it is rerouted to mouse events,
    // documented in QTabletEvent Class Reference:
    /* The event handler QWidget::tabletEvent() receives all three types of tablet events.
     Qt will first send a tabletEvent then, if it is not accepted, it will send a mouse event. */
    //
    // This is a workaround to the fact that tablet event are not delivered to child widget (like palettes)
    //

    event->setAccepted (acceptEvent);

}

bool UBBoardView::itemIsLocked(QGraphicsItem *item)
{
    if (!item)
        return false;

    return item->data(UBGraphicsItemData::ItemLocked).toBool();
}

bool UBBoardView::itemHaveParentWithType(QGraphicsItem *item, int type)
{
    if (!item)
        return false;

    if (type == item->type())
        return true;

    return itemHaveParentWithType(item->parentItem(), type);

}
bool UBBoardView::isUBItem(QGraphicsItem *item)
{
    if ((UBGraphicsItemType::UserTypesCount > item->type()) && (item->type() > QGraphicsItem::UserType))
        return true;
    else
    {
        return false;
    }
}

bool UBBoardView::isCppTool(QGraphicsItem *item)
{
    return (item->type() == UBGraphicsItemType::CompassItemType
        || item->type() == UBGraphicsItemType::RulerItemType
        || item->type() == UBGraphicsItemType::ProtractorItemType
        || item->type() == UBGraphicsItemType::TriangleItemType
        || item->type() == UBGraphicsItemType::AristoItemType
        || item->type() == UBGraphicsItemType::CurtainItemType);
}

void UBBoardView::handleItemsSelection(QGraphicsItem *item)
{
// we need to select new pressed itemOnBoard and deselect all other items.
// the trouble is in:
//                  some items can has parents (groupped items or strokes, or strokes in groups).
//                  some items is already selected and we don't need to reselect them
//
// item selection managed by QGraphicsView::mousePressEvent(). It should be called later.

    if (item)
    {
        //  item has group as first parent - it is any item or UBGraphicsStrokesGroup.
        if(item->parentItem() && UBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type())
            return;

        // delegate buttons shouldn't selected
        if (DelegateButton::Type == item->type())
            return;

        // click on svg items (images on Frame) shouldn't change selection.
        if (QGraphicsSvgItem::Type == item->type())
            return;

        // Delegate frame shouldn't selected
        if (UBGraphicsDelegateFrame::Type == item->type())
            return;


        // if we need to uwe multiple selection - we shouldn't deselect other items.
        if (!isMultipleSelectionEnabled())
        {
            // here we need to determine what item is pressed. We should work
            // only with UB items.
            if ((UBGraphicsItemType::UserTypesCount > item->type()) && (item->type() > QGraphicsItem::UserType))
            {
                // if Item can be selected at mouse press - then we need to deselect all other items.
                if(item->type() != UBGraphicsItemType::GraphicsHandle){
                    //issue 1554 - NNE - 20131009
                    scene()->deselectAllItemsExcept(item);
                }
            }
        }
    }else{
        //If item is null, it's the background, so we have to deselect all elements in the scene
        //issue 1554 - NNE - 20131009
        //scene()->deselectAllItems();
    }
}

bool UBBoardView::itemShouldReceiveMousePressEvent(QGraphicsItem *item)
{
/*
Some items should receive mouse press events averytime,
some items should receive that events when they are selected,
some items shouldn't receive mouse press events at mouse press, but should receive them at mouse release (suspended mouse press event)

Here we determines cases when items should to get mouse press event at pressing on mouse.
*/

    if (!item)
        return true;

    // for now background objects is not interactable, but it can be deprecated for some items in the future.
    if (item == scene()->backgroundObject())
        return false;

    // some behavior depends on current tool.
    UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool();    

    //EV-7 - NNE - 20140103
    if(UBShapeFactory::isShape(item)){
        if (currentTool == UBStylusTool::Play)
            return false;
        if ((currentTool == UBStylusTool::Selector) && item->isSelected())
            return true;
        if ((currentTool == UBStylusTool::Selector) && item->parentItem() && item->parentItem()->isSelected())
            return true;

        if(UBShapeFactory::isInEditMode(item)){
            return true;
        }
    }

    switch(item->type())
    {
    case UBGraphicsProtractor::Type:
    case UBGraphicsRuler::Type:
    case UBGraphicsTriangle::Type:
    case UBGraphicsCompass::Type:
    case UBGraphicsCache::Type:
    case UBGraphicsAristo::Type:
    case UBGraphicsDelegateFrame::Type:
        if (currentTool == UBStylusTool::Play)
            return false;
        return true;
    case UBGraphicsPixmapItem::Type:
    case UBGraphicsSvgItem::Type:
        if (currentTool == UBStylusTool::Play)
            return true;
        if (item->isSelected())
            return true;
        else
            return false;

    case DelegateButton::Type:
        return true;

    case UBGraphicsMediaItem::Type:
        return false;

    case UBGraphicsTextItem::Type:
        if (currentTool == UBStylusTool::Play)
            return true;
        if ((currentTool == UBStylusTool::Selector) && item->isSelected())
            return true;
        if ((currentTool == UBStylusTool::Selector) && item->parentItem() && item->parentItem()->isSelected())
            return true;
        if (currentTool != UBStylusTool::Selector)
            return false;
        break;

    case UBGraphicsItemType::StrokeItemType:
        if (currentTool == UBStylusTool::Play)
            return true;
        break;
    // Groups shouldn't reacts on any presses and moves for Play tool.
    case UBGraphicsGroupContainerItem::Type:
        if(currentTool == UBStylusTool::Play)
        {
            return true;
        }
        return false;
        break;

    case QGraphicsWebView::Type:
        return true;
    case QGraphicsProxyWidget::Type: // Issue 1313 - CFA - 20131016 : If Qt sends this unexpected type, the event should not be triggered
    {
        QGraphicsItem *c = item;

        while(c && dynamic_cast<UBGraphicsProxyWidget*>(c) == 0)
            c = c->parentItem();

        return c && dynamic_cast<UBGraphicsProxyWidget*>(c) != 0;
    }
    case UBGraphicsWidgetItem::Type:
        if (currentTool == UBStylusTool::Selector && item->parentItem() && item->parentItem()->isSelected())
            return true;
        if (currentTool == UBStylusTool::Selector && item->isSelected())
            return true;
        if (currentTool == UBStylusTool::Play)
            return true;
        return false;
        break;
    case UBGraphicsItemType::GraphicsHandle:
        return true;

    }

    return !isUBItem(item); // standard behavior of QGraphicsScene for not UB items. UB items should be managed upper.
}

bool UBBoardView::itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item)
{
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool();

    switch(item->type())
    {
    case QGraphicsWebView::Type:
        return false;
    case UBGraphicsPixmapItem::Type:
    case UBGraphicsSvgItem::Type:
    case UBGraphicsTextItem::Type:
    case UBGraphicsWidgetItem::Type:
        if (currentTool == UBStylusTool::Selector && !item->isSelected() && item->parentItem())
            return true;
        if (currentTool == UBStylusTool::Selector && item->isSelected())
            return true;
        break;

    case DelegateButton::Type:
    case UBGraphicsMediaItem::Type:
        return true;
    }

    return false;

}

bool UBBoardView::itemShouldBeMoved(QGraphicsItem *item)
{
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    if (!(mMouseButtonIsPressed || mTabletStylusIsPressed))
        return false;

    if (movingItem->data(UBGraphicsItemData::ItemLocked).toBool())
        return false;

    if (movingItem->parentItem() && UBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type() && !movingItem->isSelected() && movingItem->parentItem()->isSelected())
        return false;

    if (movingItem->parentItem() && UBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type())
        if(dynamic_cast<UBGraphicsGroupContainerItem*>(movingItem->parentItem())->Delegate()->isLocked())
            return false;

    UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool();

    //EV-7 - NNE - 20140103
    if(UBShapeFactory::isShape(item)){
        //if the item is selected or in edit mode
        //we have to received the mouse event throught the QGraphicsView
        if(item->isSelected() || UBShapeFactory::isInEditMode(item)){
            return false;
        }
        return true;
    }

    switch(item->type())
    {
    case UBGraphicsCurtainItem::Type:
        return true;
    case UBGraphicsGroupContainerItem::Type:
        return !dynamic_cast<UBGraphicsGroupContainerItem*>(item)->Delegate()->isLocked();
    case UBGraphicsWidgetItem::Type:
    {
        if(currentTool == UBStylusTool::Selector && item->isSelected())
            return false;
        UBGraphicsWidgetItem* widgetItem= dynamic_cast<UBGraphicsWidgetItem*>(item);

        if(currentTool == UBStylusTool::Play && widgetItem && !widgetItem->isFeatureRTE())
            return false;
    }
    case UBGraphicsSvgItem::Type:
    case UBGraphicsPixmapItem::Type:
        if (currentTool == UBStylusTool::Play || !item->isSelected())
            return true;
        if (item->isSelected())
            return false;
    case UBGraphicsMediaItem::Type:
    case UBGraphicsStrokesGroup::Type:
        return true;
    case UBGraphicsTextItem::Type:
        return !item->isSelected();
    }

    return false;
}


QGraphicsItem* UBBoardView::determineItemToPress(QGraphicsItem *item)
{
    if(item)
    {
        UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool();

        //TODO claudio
        // another chuck of very good code        
        if(item->parentItem() && UBGraphicsGroupContainerItem::Type == item->parentItem()->type() && currentTool == UBStylusTool::Play){
            UBGraphicsGroupContainerItem* group = qgraphicsitem_cast<UBGraphicsGroupContainerItem*>(item->parentItem());
            if(group && group->Delegate()->action()){
                group->Delegate()->action()->play();
                return item;
            }
        }

        // if item is on group and group is not selected - group should take press.
        if ((UBStylusTool::Selector == currentTool
             || currentTool == UBStylusTool::Play) // Issue 1509 - AOU - 20131113
            && item->parentItem()
            && UBGraphicsGroupContainerItem::Type == item->parentItem()->type())
                /*&& !item->parentItem()->isSelected())*/ // Issue 1509 - AOU - 20131113
        {
            return item->parentItem();
        }

        // items like polygons placed in two groups nested, so we need to recursive call.
        if(item->parentItem() && UBGraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToPress(item->parentItem());
    }

    return item;
}

// determine item to interacts: item self or it's container.
QGraphicsItem* UBBoardView::determineItemToMove(QGraphicsItem *item)
{
    if(item)
    {
        UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController()->stylusTool();

        //W3C widgets should take mouse move events from play tool.
        if ((UBStylusTool::Play == currentTool) && (UBGraphicsWidgetItem::Type == item->type()))
                return item;

        // if item is in group
        if(item->parentItem() && UBGraphicsGroupContainerItem::Type == item->parentItem()->type())
        {
            // play tool should move groups by any element
            if (UBStylusTool::Play == currentTool && item->parentItem()->isSelected())
                return item->parentItem();

            // groups should should be moved instead of strokes groups
            if (UBGraphicsStrokesGroup::Type == item->type())
                return item->parentItem();

            // selected groups should be moved by moving any element
            if (item->parentItem()->isSelected())
                return item;

            if (item->isSelected())
                return NULL;

            return item->parentItem();
        }

        // items like polygons placed in two groups nested, so we need to recursive call.
        if(item->parentItem() && UBGraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToMove(item->parentItem());
    }

    return item;
}

void UBBoardView::handleItemMousePress(QMouseEvent *event)
{
    mLastPressedMousePos = mapToScene(event->pos());

    // Determining item who will take mouse press event
    //all other items will be deselected and if all item will be deselected, then
    // wrong item can catch mouse press. because selected items placed on the top
    movingItem = determineItemToPress(movingItem);
    handleItemsSelection(movingItem);

    if (isMultipleSelectionEnabled())
        return;

    if (itemShouldReceiveMousePressEvent(movingItem))
    {
        QGraphicsView::mousePressEvent (event);

        // Issue 1313 - CFA - 20131022 : In some cases, mousePressEvent alters graphics items position
        QGraphicsItem* item = determineItemToPress(scene()->itemAt(this->mapToScene(event->posF().toPoint()), transform()));
        //use QGraphicsView::transorm() to use not deprecated QGraphicsScene::itemAt() method

        if (item && (item->type() == QGraphicsProxyWidget::Type) && item->parentObject() && item->parentObject()->type() != QGraphicsProxyWidget::Type)
        {
            //Clean up children
            QList<QGraphicsItem*> children = item->children();

            for( QList<QGraphicsItem*>::iterator it = children.begin(); it != children.end(); ++it )
                if ((*it)->pos().x() < 0 || (*it)->pos().y() < 0)
                    (*it)->setPos(0,item->boundingRect().size().height());
        }
        // FIN Issue 1313 - CFA - 20131022
    }
    else
    {
        if (movingItem)
        {
            UBGraphicsItem *graphicsItem = dynamic_cast<UBGraphicsItem*>(movingItem);
            if (graphicsItem)
                graphicsItem->Delegate()->startUndoStep();

            movingItem->clearFocus();
        }

        if (suspendedMousePressEvent)
        {
            delete suspendedMousePressEvent;
            suspendedMousePressEvent = NULL;
        }

        if (itemShouldReceiveSuspendedMousePressEvent(movingItem))
        {
            suspendedMousePressEvent = new QMouseEvent(event->type(), event->pos(), event->button(), event->buttons(), event->modifiers());
        }
    }
}

void UBBoardView::handleItemMouseMove(QMouseEvent *event)
{
    // determine item to move (maybee we need to move group of item or his parent.
    movingItem = determineItemToMove(movingItem);

    // items should be moved not every mouse move.
    if (movingItem && itemShouldBeMoved(movingItem) && (mMouseButtonIsPressed || mTabletStylusIsPressed))
    {
        QPointF scenePos = mapToScene(event->pos());
        QPointF newPos = movingItem->pos() + scenePos - mLastPressedMousePos;
        movingItem->setPos(newPos);
        mLastPressedMousePos = scenePos;
        mWidgetMoved = true;

        event->accept();
    }
    else
    {
        QPointF posBeforeMove;
        QPointF posAfterMove;

        if (movingItem)
            posBeforeMove = movingItem->pos();

        QGraphicsView::mouseMoveEvent (event);


        if (movingItem)
          posAfterMove = movingItem->pos();

        mWidgetMoved = ((posAfterMove-posBeforeMove).manhattanLength() != 0);

        // a cludge for terminate moving of w3c widgets.
        // in some cases w3c widgets catches mouse move and doesn't sends that events to web page,
        // at simple - in google map widget - mouse move events doesn't comes to web page from rectangle of wearch bar on bottom right corner of widget.
        if (mWidgetMoved && (UBGraphicsW3CWidgetItem::Type == movingItem->type() || UBGraphicsGroupContainerItem::Type == movingItem->type()))
            movingItem->setPos(posBeforeMove);
    }
}

void UBBoardView::rubberItems()
{
    if (mUBRubberBand)
        mRubberedItems = items(mUBRubberBand->geometry());

    foreach(QGraphicsItem *item, mRubberedItems)
    {
        if (item->parentItem() && UBGraphicsGroupContainerItem::Type == item->parentItem()->type())
            mRubberedItems.removeOne(item);
    }
}

void UBBoardView::moveRubberedItems(QPointF movingVector)
{
    QRectF invalidateRect = scene()->itemsBoundingRect();

    foreach (QGraphicsItem *item, mRubberedItems)
    {

        if (item->type() == UBGraphicsW3CWidgetItem::Type
              || item->type() == UBGraphicsPixmapItem::Type
              || item->type() == UBGraphicsMediaItem::Type
              || item->type() == UBGraphicsSvgItem::Type
              || item->type() == UBGraphicsTextItem::Type
              || item->type() == UBGraphicsStrokesGroup::Type
              || item->type() == UBGraphicsGroupContainerItem::Type)
        {
              item->setPos(item->pos()+movingVector);
        }
    }

    scene()->invalidate(invalidateRect);
}

void UBBoardView::setMultiselection(bool enable)
{
    mMultipleSelectionIsEnabled = enable;
}

// work around for handling tablet events on MAC OS with Qt 4.8.0 and above
#if defined(Q_WS_MACX)
bool UBBoardView::directTabletEvent(QEvent *event)
{
    QTabletEvent *tEvent = static_cast<QTabletEvent *>(event);
    tEvent = new QTabletEvent(tEvent->type()
        , mapFromGlobal(tEvent->pos())
        , tEvent->globalPos()
        , tEvent->hiResGlobalPos()
        , tEvent->device()
        , tEvent->pointerType()
        , tEvent->pressure()
        , tEvent->xTilt()
        , tEvent->yTilt()
        , tEvent->tangentialPressure()
        , tEvent->rotation()
        , tEvent->z()
        , tEvent->modifiers()
        , tEvent->uniqueId());

    if (geometry().contains(tEvent->pos()))
    {
        if (NULL == widgetForTabletEvent(this->parentWidget(), tEvent->pos()))
        {
            tabletEvent(tEvent);
            return true;
        }
    }
    return false;
}

QWidget *UBBoardView::widgetForTabletEvent(QWidget *w, const QPoint &pos)
{
    Q_ASSERT(w);

    // it should work that, but it doesn't. So we check if it is control view.
    //UBBoardView *board = qobject_cast<UBBoardView *>(w);
    UBBoardView *board = UBApplication::boardController->controlView();

    QWidget *childAtPos = NULL;

    QList<QObject *> childs = w->children();
    foreach(QObject *child, childs)
    {
        QWidget *childWidget = qobject_cast<QWidget *>(child);
        if (childWidget)
        {
            if (childWidget->isVisible() && childWidget->geometry().contains(pos))
            {
                QWidget *lastChild = widgetForTabletEvent(childWidget, pos);

                if (board && board->viewport() == lastChild)
                    continue;

                if (NULL != lastChild)
                    childAtPos = lastChild;
                else
                    childAtPos = childWidget;

                break;
            }
            else
                childAtPos = NULL;
        }
    }
    return childAtPos;
}
#endif
void UBBoardView::longPressEvent()
{
   UBDrawingController *drawingController = UBDrawingController::drawingController();
   UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController ()->stylusTool ();

   disconnect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));

   if (UBStylusTool::Selector == currentTool)
   {
        drawingController->setStylusTool(UBStylusTool::Play);
   }
   else
   if (currentTool == UBStylusTool::Play)
   {
        drawingController->setStylusTool(UBStylusTool::Selector);
   }
   else
   if (UBStylusTool::Eraser == currentTool)
   {
       UBApplication::boardController->paletteManager()->toggleErasePalette(true);
   }

}

void UBBoardView::mousePressEvent (QMouseEvent *event)
{
    //EV-7 - NNE - 20131231
    emit mousePress(event);

    if (!bIsControl && !bIsDesktop) {
        event->ignore();
        return;
    }

    mIsDragInProgress = false;

    if (isAbsurdPoint (event->pos ()))
    {
        event->accept ();
        return;
    }

    mMouseDownPos = event->pos ();

    movingItem = scene()->itemAt(this->mapToScene(event->posF().toPoint()));

    if (!movingItem)
        emit clickOnBoard();

    if (event->button () == Qt::LeftButton && isInteractive ())
    {

        UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController ()->stylusTool ();

        if (!mTabletStylusIsPressed)
            mMouseButtonIsPressed = true;

        if (currentTool == UBStylusTool::ZoomIn)
        {
            mController->zoomIn (mapToScene (event->pos ()));
            event->accept ();
        }
        else if (currentTool == UBStylusTool::ZoomOut)
        {
            mController->zoomOut (mapToScene (event->pos ()));
            event->accept ();
        }
        else if (currentTool == UBStylusTool::Hand)
        {
            viewport ()->setCursor (QCursor (Qt::ClosedHandCursor));
            mPreviousPoint = event->posF ();
            event->accept ();
        }
        else if (currentTool == UBStylusTool::Selector || currentTool == UBStylusTool::Play)
        {
            if (bIsDesktop) {
                event->ignore();
                return;
            }

            if (currentTool == UBStylusTool::Play)
            {
                // Issue retours 2.4RC1 - CFA - 20140217 : No idea why "play action" is doing in determineItemToPress...)
                UBAbstractGraphicsItem* shape = dynamic_cast<UBAbstractGraphicsItem*>(movingItem);
                if (shape && shape->Delegate() && shape->Delegate()->action())
                    shape->Delegate()->action()->play();
            }

            if (scene()->backgroundObject() == movingItem)
                movingItem = NULL;

            connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));
            if (!movingItem && !mController->cacheIsVisible())
                mLongPressTimer.start();

            if (!movingItem) {
                // Rubberband selection implementation
                if (!mUBRubberBand) {
                    mUBRubberBand = new UBRubberBand(QRubberBand::Rectangle, this);
                }
                mUBRubberBand->setGeometry (QRect (mMouseDownPos, QSize ()));
                mUBRubberBand->show();
            }
            else
            {
                if(mUBRubberBand)
                    mUBRubberBand->hide();
            }

            handleItemMousePress(event);
            event->accept();
        }
        else if (currentTool == UBStylusTool::Text)
        {
            int frameWidth = UBSettings::settings ()->objectFrameWidth;
            QRectF fuzzyRect (0, 0, frameWidth * 4, frameWidth * 4);
            fuzzyRect.moveCenter (mapToScene (mMouseDownPos));

            UBGraphicsTextItem* foundTextItem = 0;
            QListIterator<QGraphicsItem *> it (scene ()->items (fuzzyRect));

            while (it.hasNext () && !foundTextItem)
            {
                foundTextItem = qgraphicsitem_cast<UBGraphicsTextItem*>(it.next ());
            }

            if (foundTextItem)
            {
                mIsCreatingTextZone = false;
                QGraphicsView::mousePressEvent (event);
            }
            else
            {
                scene ()->deselectAllItems ();

                if (!mRubberBand)
                    mRubberBand = new UBRubberBand (QRubberBand::Rectangle, this);
                mRubberBand->setGeometry (QRect (mMouseDownPos, QSize ()));
                mRubberBand->show ();
                mIsCreatingTextZone = true;

                event->accept ();
            }
        }
        else if (currentTool == UBStylusTool::RichText)
        {

        }
        else if (currentTool == UBStylusTool::Capture)
        {
            scene ()->deselectAllItems();

            if (!mRubberBand)
                mRubberBand = new UBRubberBand (QRubberBand::Rectangle, this);

            mRubberBand->setGeometry (QRect (mMouseDownPos, QSize ()));
            mRubberBand->show ();
            mIsCreatingSceneGrabZone = true;

            event->accept ();
        }
        else if (currentTool == UBStylusTool::ChangeFill)
        {
            qDebug() << "on est dans le cas du pot de peinture, on va remplir l'objet si possible";
            UBApplication::boardController->shapeFactory().changeFillColor(mapToScene(mMouseDownPos));             
        }
        else
        {
            if(UBDrawingController::drawingController()->mActiveRuler==NULL)
            {
                viewport()->setCursor (QCursor (Qt::BlankCursor));
            }

            if (scene () && !mTabletStylusIsPressed)
            {
                if (currentTool == UBStylusTool::Eraser)
                {
                    connect(&mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressEvent()));
                    mLongPressTimer.start();
                }
                scene ()->inputDevicePress (mapToScene (UBGeometryUtils::pointConstrainedInRect (event->pos (), rect ())));
            }
            event->accept ();
        }
    }
}

void
UBBoardView::mouseMoveEvent (QMouseEvent *event)
{

    //EV-7 - NNE - 20131231
    emit mouseMove(event);

  if(!mIsDragInProgress && ((mapToScene(event->pos()) - mLastPressedMousePos).manhattanLength() < QApplication::startDragDistance()))
  {
      return;
  }

  mIsDragInProgress = true;
  UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController ()->stylusTool ();

  mLongPressTimer.stop();

  if (isAbsurdPoint (event->pos ()))
    {
      event->accept ();
      return;
    }

  if (currentTool == UBStylusTool::Hand && (mMouseButtonIsPressed || mTabletStylusIsPressed))
    {
      QPointF eventPosition = event->posF ();
      qreal dx = eventPosition.x () - mPreviousPoint.x ();
      qreal dy = eventPosition.y () - mPreviousPoint.y ();
      mController->handScroll (dx, dy);
      mPreviousPoint = eventPosition;
      event->accept ();
    }
  else if (currentTool == UBStylusTool::Selector || currentTool == UBStylusTool::Play)
  {
      if((event->pos() - mLastPressedMousePos).manhattanLength() < QApplication::startDragDistance()) {
          return;
      }

      if (bIsDesktop) {
          event->ignore();
          return;
      }

      if(currentTool == UBStylusTool::Play && movingItem->data(UBGraphicsItemData::ItemLocked).toBool()){
          event->accept();
          return;
      }

      if (currentTool != UBStylusTool::Play || mRubberBandInPlayMode) {

          if (!movingItem && (mMouseButtonIsPressed || mTabletStylusIsPressed) && mUBRubberBand && mUBRubberBand->isVisible()) {

              QRect bandRect(mMouseDownPos, event->pos());

              bandRect = bandRect.normalized();

              mUBRubberBand->setGeometry(bandRect);

              QList<QGraphicsItem *> rubberItems = items(bandRect);
              if (currentTool == UBStylusTool::Selector)
              {
                  foreach (QGraphicsItem *item, items())
                  {
                      // Issue 1569 - CFA - 20131113 : le traitement spécifique aux polygones (fait partout ailleurs) n'était pas fait ici
                      if (item->type() == UBGraphicsItemType::PolygonItemType)
                            if (item->parentItem())
                                item = item->parentItem();
                      // Fin Issue 1569 - CFA - 20131113

                      //EV-7 - NNE - 20140103 : Add test on drawing objects
                      if (item->type() == UBGraphicsW3CWidgetItem::Type
                              || item->type() == UBGraphicsPixmapItem::Type
                              || item->type() == UBGraphicsMediaItem::Type
                              || item->type() == UBGraphicsSvgItem::Type
                              || item->type() == UBGraphicsTextItem::Type
                              || item->type() == UBGraphicsStrokesGroup::Type
                              || item->type() == UBGraphicsGroupContainerItem::Type
                              || UBShapeFactory::isShape(item))
                      {
                          if (rubberItems.contains(item))
                              item->setSelected(true);
                          else
                              item->setSelected(false);
                      }
                  }
              }
          }
      }

      handleItemMouseMove(event);
    }
  else if ((UBDrawingController::drawingController()->isDrawingTool())
    && !mMouseButtonIsPressed)
  {
      QGraphicsView::mouseMoveEvent (event);
  }
  else if (currentTool == UBStylusTool::Text || currentTool == UBStylusTool::Capture)
    {
      if (mRubberBand && (mIsCreatingTextZone || mIsCreatingSceneGrabZone))
        {
          mRubberBand->setGeometry (QRect (mMouseDownPos, event->pos ()).normalized ());
          event->accept ();
        }
      else
        {
          QGraphicsView::mouseMoveEvent (event);
        }
    }
  else
    {
      if (!mTabletStylusIsPressed && scene ())
      {
          scene ()->inputDeviceMove (mapToScene (UBGeometryUtils::pointConstrainedInRect (event->pos (), rect ())), mMouseButtonIsPressed);
      }
      event->accept ();
    }

  if((event->pos() - mLastPressedMousePos).manhattanLength() < QApplication::startDragDistance())
      mWidgetMoved = true;
}

void
UBBoardView::mouseReleaseEvent (QMouseEvent *event)
{
    //EV-7 - NNE - 20131231
    emit mouseRelease(event);

    UBStylusTool::Enum currentTool = (UBStylusTool::Enum)UBDrawingController::drawingController ()->stylusTool ();

  setToolCursor (currentTool);
  // first/ propagate device release to the scene
  if (scene ())
    scene ()->inputDeviceRelease ();

  if (currentTool == UBStylusTool::Selector)
  {
      if (bIsDesktop) {
          event->ignore();
          return;
      }

      UBGraphicsItem *graphicsItem = dynamic_cast<UBGraphicsItem*>(movingItem);
      if (graphicsItem)
          graphicsItem->Delegate()->commitUndoStep();

      bool bReleaseIsNeed = true;
      if (movingItem != determineItemToPress(scene()->itemAt(this->mapToScene(event->posF().toPoint()))))
      {
          movingItem = NULL;
          bReleaseIsNeed = false;
      }
      if (mWidgetMoved)
      {
          mWidgetMoved = false;
          movingItem = NULL;
      }
      else
      if (movingItem && (!isCppTool(movingItem) || UBGraphicsCurtainItem::Type == movingItem->type()))
      {
          if (suspendedMousePressEvent)
          {
              QGraphicsView::mousePressEvent(suspendedMousePressEvent);     // suspendedMousePressEvent is deleted by old Qt event loop
              movingItem = NULL;
              delete suspendedMousePressEvent;
              suspendedMousePressEvent = NULL;
              bReleaseIsNeed = true;
          }
          else
          {
             if (isUBItem(movingItem) &&
                DelegateButton::Type != movingItem->type() &&
                UBGraphicsDelegateFrame::Type !=  movingItem->type() &&
                UBGraphicsCache::Type != movingItem->type() &&
                QGraphicsWebView::Type != movingItem->type() && // for W3C widgets as Tools.
                !(!isMultipleSelectionEnabled() && movingItem->parentItem() && UBGraphicsWidgetItem::Type == movingItem->type() && UBGraphicsGroupContainerItem::Type == movingItem->parentItem()->type()))
             {
                 bReleaseIsNeed = false;
                 if (movingItem->isSelected() && isMultipleSelectionEnabled())
                    movingItem->setSelected(false);
                 else
                 if (movingItem->parentItem() && movingItem->parentItem()->isSelected() && isMultipleSelectionEnabled())
                    movingItem->parentItem()->setSelected(false);
                 else
                 {
                    if (movingItem->isSelected())
                        bReleaseIsNeed = true;

                    movingItem->setSelected(true);
                 }

             }
          }
      }
      else
          bReleaseIsNeed = true;

      if (mUBRubberBand && mUBRubberBand->isVisible()) {
          mUBRubberBand->hide();
      }

      if (bReleaseIsNeed)
      {
          QGraphicsView::mouseReleaseEvent (event);
      }
  }
  else if (currentTool == UBStylusTool::Play)
  {
      if (bIsDesktop) {
          event->ignore();
          return;
      }

      //Issue 1541 - AOU - 20131002
      UBGraphicsItem *graphicsItem = dynamic_cast<UBGraphicsItem*>(movingItem);
      if (graphicsItem)
          graphicsItem->Delegate()->commitUndoStep();
      //Issue 1541 - AOU - 20131002 : Fin

      if (mWidgetMoved)
      {
          movingItem = NULL;
          mWidgetMoved = false;
      }
      else
      {
          if (suspendedMousePressEvent)
          {
              QGraphicsView::mousePressEvent(suspendedMousePressEvent);     // suspendedMousePressEvent is deleted by old Qt event loop
              movingItem = NULL;
              delete suspendedMousePressEvent;
              suspendedMousePressEvent = NULL;
          }
      }
      QGraphicsView::mouseReleaseEvent (event);
  }
  else if (currentTool == UBStylusTool::Text)
    {
      if (mRubberBand)
        mRubberBand->hide ();

      if (scene () && mRubberBand && mIsCreatingTextZone)
        {
          QRect rubberRect = mRubberBand->geometry ();

          UBGraphicsTextItem* textItem = scene()->addTextHtml ("", mapToScene (rubberRect.topLeft ()));
          event->accept ();

          UBDrawingController::drawingController ()->setStylusTool (UBStylusTool::Selector);

          textItem->setSelected (true);
          textItem->setFocus();
        }
      else
        {
          QGraphicsView::mouseReleaseEvent (event);
        }

      mIsCreatingTextZone = false;
    }
  else if (currentTool == UBStylusTool::RichText)
  {
      if (mRubberBand)
        mRubberBand->hide ();

       UBFeaturesController* c = UBApplication::boardController->paletteManager()->featuresWidget()->getFeaturesController();
       UBFeature f = c->getFeatureByPath("/root/Applications/Texte Enrichi.wgt" );
       UBApplication::boardController->downloadURL(f.getFullPath(), QString(), mapToScene (event->pos ()) );

       QGraphicsItem* widget = scene()->itemAt(this->mapToScene(event->posF().toPoint()), transform());

       if (widget)
           widget->setFocus();

       UBDrawingController::drawingController ()->setStylusTool (UBStylusTool::Selector);

  }
  else if (currentTool == UBStylusTool::Capture)
    {
      if (mRubberBand)
        mRubberBand->hide ();

      if (scene () && mRubberBand && mIsCreatingSceneGrabZone && mRubberBand->geometry ().width () > 16)
        {
          QRect rect = mRubberBand->geometry ();
          QPointF sceneTopLeft = mapToScene (rect.topLeft ());
          QPointF sceneBottomRight = mapToScene (rect.bottomRight ());
          QRectF sceneRect (sceneTopLeft, sceneBottomRight);

          mController->grabScene (sceneRect);

          event->accept ();
        }
      else
        {
          QGraphicsView::mouseReleaseEvent (event);
        }

      mIsCreatingSceneGrabZone = false;
    }
  else
    {
      if (mPendingStylusReleaseEvent || mMouseButtonIsPressed)
        {
          event->accept ();
        }
    }

  mMouseButtonIsPressed = false;
  mPendingStylusReleaseEvent = false;
  mTabletStylusIsPressed = false;
  movingItem = NULL;

  mLongPressTimer.stop();

  UBApplication::boardController->controlView()->viewport()->update(); // Issue 1026 - ALTI - 20131024 : depuis que le thumbnail courant est une view "live" de la boardScene, il peut y avoir des "trainées" quand on déplace rapidement les objets. Il faut rafraichir en fin de déplacement.
}

void
UBBoardView::forcedTabletRelease ()
{

  if (mMouseButtonIsPressed || mTabletStylusIsPressed || mPendingStylusReleaseEvent)
    {
      qWarning () << "dirty mouse/tablet state:";
      qWarning () << "mMouseButtonIsPressed =" << mMouseButtonIsPressed;
      qWarning () << "mTabletStylusIsPressed = " << mTabletStylusIsPressed;
      qWarning () << "mPendingStylusReleaseEvent" << mPendingStylusReleaseEvent;
      qWarning () << "forcing device release";

      scene ()->inputDeviceRelease ();

      mMouseButtonIsPressed = false;
      mTabletStylusIsPressed = false;
      mPendingStylusReleaseEvent = false;
    }
}

void
UBBoardView::mouseDoubleClickEvent (QMouseEvent *event)
{
    //Issue retours 2.4RC1 - CFA - 20140218 : handle W3CWidgetItems doubleClick
    QGraphicsItem* item = determineItemToPress(scene()->itemAt(this->mapToScene(event->posF().toPoint()), transform()));

    if (item && (item->type() == UBGraphicsW3CWidgetItem::Type))
        QGraphicsView::mouseDoubleClickEvent(event);
    else
    {
        // We don't want a double click, we want two clicks ...
        mousePressEvent (event);
    }
}

void
UBBoardView::wheelEvent (QWheelEvent *wheelEvent)
{
  if (isInteractive () && wheelEvent->orientation () == Qt::Vertical)
    {
      // Too many wheelEvent are sent, how should we handle them to "smoothly" zoom ?
      // something like zoom( pow(zoomFactor, event->delta() / 120) )

      // use DateTime man, store last event time, and if if less than 300ms than this is one big scroll
      // and move scroll with one const speed.
        // so, you no will related with scroll event count
    }

    QList<QGraphicsItem *> selItemsList = scene()->selectedItems();
    // if NO have selected items, than no need process mouse wheel. just exist
    if( selItemsList.count() > 0 )
    {
        // only one selected item possible, so we will work with first item only
        QGraphicsItem * selItem = selItemsList[0];

        // get items list under mouse cursor
        QPointF scenePos = mapToScene(wheelEvent->pos());
        QList<QGraphicsItem *> itemsList = scene()->items(scenePos);

        QBool isSlectedAndMouseHower = itemsList.contains(selItem);
        if(isSlectedAndMouseHower)
        {
            QGraphicsView::wheelEvent(wheelEvent);
            wheelEvent->accept();
        }

    }

}

void
UBBoardView::leaveEvent (QEvent * event)
{
  if (scene ())
    scene ()->leaveEvent (event);

  QGraphicsView::leaveEvent (event);
}

void UBBoardView::drawItems (QPainter *painter, int numItems,
                        QGraphicsItem* items[],
                        const QStyleOptionGraphicsItem options[])
{
  if (!mFilterZIndex)
    {
      QGraphicsView::drawItems (painter, numItems, items, options);
    }
  else
    {
      int count = 0;

      QGraphicsItem** itemsFiltered = new QGraphicsItem*[numItems];
      QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

      for (int i = 0; i < numItems; i++)
        {
          if (shouldDisplayItem (items[i]))
            {
              itemsFiltered[count] = items[i];
              optionsFiltered[count] = options[i];
              count++;
            }
        }

      QGraphicsView::drawItems (painter, count, itemsFiltered, optionsFiltered);

      delete[] optionsFiltered;
      delete[] itemsFiltered;
    }
}


void UBBoardView::dragMoveEvent(QDragMoveEvent *event)
{
  QGraphicsView::dragMoveEvent(event);
  event->acceptProposedAction();
}

void UBBoardView::dropEvent (QDropEvent *event)
{
    QGraphicsItem *onItem = itemAt(event->pos().x(),event->pos().y());

    //N/C - NNE - 20140303 : add test for the RTE widget
    bool isUBGraphicsWidget = onItem && onItem->type() == UBGraphicsWidgetItem::Type;
    UBGraphicsWidgetItem *item = 0;
    UBGraphicsTextItem* textItem = NULL;

    if(onItem)
    {
        item = dynamic_cast<UBGraphicsWidgetItem*>(onItem);
        textItem = dynamic_cast<UBGraphicsTextItem*>(onItem);
    }

    UBFeature f = UBApplication::boardController->paletteManager()->featuresWidget()->getCentralWidget()->getCurElementFromProperties();
    //Ev-NC - CFA - 20140403 : now we can use thumbnails to drop images
    UBDraggableThumbnail* droppedThumbnail = dynamic_cast<UBDraggableThumbnail*>(event->source());
    if (textItem && textItem->isSelected())
    {
        if (droppedThumbnail)
        {
            textItem->insertImage(f.getFullPath().toLocalFile());
            return;
        }
        else
        {
            if (event->mimeData()->hasUrls())
            {
                QList<QUrl> urls = event->mimeData()->urls();

                const UBFeaturesMimeData *internalMimeData = qobject_cast<const UBFeaturesMimeData*>(event->mimeData());
                if (internalMimeData)
                {
                    foreach (QUrl url, urls)
                        textItem->insertImage(url.toLocalFile());
                }
                return;
            }
        }
    }
    else
    {
        if (droppedThumbnail)
        {
             UBApplication::boardController->downloadURL(f.getFullPath(), QString(), mapToScene (event->pos ()) );
             return;
        }
    }

    //take care about the lazy evaluation of the test
    bool isFeatureRTE = isUBGraphicsWidget
            && item
            && item->isFeatureRTE();
    //N/C - NNE - 20140303 : END

    if ((isUBGraphicsWidget && !isFeatureRTE) || (isFeatureRTE && onItem->isSelected())) {
        QGraphicsView::dropEvent(event);
    } else {
        if (!event->source()
                || qobject_cast<UBThumbnailWidget *>(event->source())
                || qobject_cast<QWebView*>(event->source())
                || qobject_cast<UBTGMediaWidget*>(event->source())
                || qobject_cast<QListView *>(event->source())
                || qobject_cast<UBTGDraggableTreeItem*>(event->source())) {
            mController->processMimeData (event->mimeData (), mapToScene (event->pos ()));
            event->acceptProposedAction();
        }
    }
    //prevent features in UBFeaturesWidget deletion from the model when event is processing inside
    //Qt base classes
    if (event->dropAction() == Qt::MoveAction) {
        event->setDropAction(Qt::CopyAction);
    }
}

void UBBoardView::resizeEvent (QResizeEvent * event)
{
  const qreal maxWidth = width () * 10;
  const qreal maxHeight = height () * 10;

  setSceneRect (-(maxWidth / 2), -(maxHeight / 2), maxWidth, maxHeight);
  centerOn (0, 0);

  emit resized (event);
}

void UBBoardView::drawBackground (QPainter *painter, const QRectF &rect)
{
  if (testAttribute (Qt::WA_TranslucentBackground))
    {
      QGraphicsView::drawBackground (painter, rect);
      return;
    }

  bool darkBackground = scene () && scene ()->isDarkBackground ();

  if (darkBackground)
    {
      painter->fillRect (rect, QBrush (QColor (Qt::black)));
    }
  else
    {
      painter->fillRect (rect, QBrush (QColor (Qt::white)));
    }

  if (transform ().m11 () > 0.5)
    {
      QColor bgCrossColor;

      if (darkBackground)
        bgCrossColor = UBSettings::crossDarkBackground;
      else
        bgCrossColor = UBSettings::crossLightBackground;

      if (transform ().m11 () < 1.0)
        {
          int alpha = 255 * transform ().m11 () / 2;
          bgCrossColor.setAlpha (alpha); // fade the crossing on small zooms
        }

      painter->setPen (bgCrossColor);

      if (scene () && scene ()->isCrossedBackground ())
        {
          qreal firstY = ((int) (rect.y () / UBSettings::crossSize)) * UBSettings::crossSize;

          for (qreal yPos = firstY; yPos < rect.y () + rect.height (); yPos += UBSettings::crossSize)
            {
              painter->drawLine (rect.x (), yPos, rect.x () + rect.width (), yPos);
            }

          qreal firstX = ((int) (rect.x () / UBSettings::crossSize)) * UBSettings::crossSize;

          for (qreal xPos = firstX; xPos < rect.x () + rect.width (); xPos += UBSettings::crossSize)
            {
              painter->drawLine (xPos, rect.y (), xPos, rect.y () + rect.height ());
            }
        }
    }

  if (!mFilterZIndex && scene ())
    {
      QSize pageNominalSize = scene ()->nominalSize ();

      if (pageNominalSize.isValid ())
        {
          qreal penWidth = 8.0 / transform ().m11 ();

          QRectF pageRect (pageNominalSize.width () / -2, pageNominalSize.height () / -2
                           , pageNominalSize.width (), pageNominalSize.height ());

          pageRect.adjust (-penWidth / 2, -penWidth / 2, penWidth / 2, penWidth / 2);

          QColor docSizeColor;

          if (darkBackground)
            docSizeColor = UBSettings::documentSizeMarkColorDarkBackground;
          else
            docSizeColor = UBSettings::documentSizeMarkColorLightBackground;

          QPen pen (docSizeColor);
          pen.setWidth (penWidth);
          painter->setPen (pen);
          painter->drawRect (pageRect);
        }
    }
}

void UBBoardView::settingChanged (QVariant newValue)
{
  Q_UNUSED (newValue);

  mPenPressureSensitive = UBSettings::settings ()->boardPenPressureSensitive->get ().toBool ();
  mMarkerPressureSensitive = UBSettings::settings ()->boardMarkerPressureSensitive->get ().toBool ();
  mUseHighResTabletEvent = UBSettings::settings ()->boardUseHighResTabletEvent->get ().toBool ();
}

void UBBoardView::virtualKeyboardActivated(bool b)
{
    UBPlatformUtils::setWindowNonActivableFlag(this, b);
    mVirtualKeyboardActive = b;
    setInteractive(!b);
}


// Apple remote desktop sends funny events when the transmission is bad

bool UBBoardView::isAbsurdPoint(QPoint point)
{
    QDesktopWidget *desktop = qApp->desktop ();
    bool isValidPoint = false;

    for (int i = 0; i < desktop->numScreens (); i++)
    {
      QRect screenRect = desktop->screenGeometry (i);
      isValidPoint = isValidPoint || screenRect.contains (mapToGlobal(point));
    }

    return !isValidPoint;
}

void UBBoardView::focusOutEvent (QFocusEvent * event)
{
  Q_UNUSED (event);
}

void UBBoardView::setToolCursor (int tool)
{
  QWidget *controlViewport = viewport ();
  switch (tool)
    {
    case UBStylusTool::Pen:
      controlViewport->setCursor (UBResources::resources ()->penCursor);
      break;
    case UBStylusTool::Eraser:
      controlViewport->setCursor (UBResources::resources ()->eraserCursor);
      scene()->hideEraser();
      break;
    case UBStylusTool::Marker:
      controlViewport->setCursor (UBResources::resources ()->markerCursor);
      break;
    case UBStylusTool::Pointer:
      controlViewport->setCursor (UBResources::resources ()->pointerCursor);
      break;
    case UBStylusTool::Hand:
      controlViewport->setCursor (UBResources::resources ()->handCursor);
      break;
    case UBStylusTool::ZoomIn:
      controlViewport->setCursor (UBResources::resources ()->zoomInCursor);
      break;
    case UBStylusTool::ZoomOut:
      controlViewport->setCursor (UBResources::resources ()->zoomOutCursor);
      break;
    case UBStylusTool::Selector:
      controlViewport->setCursor (UBResources::resources ()->arrowCursor);
      break;
    case UBStylusTool::Play:
      controlViewport->setCursor (UBResources::resources ()->playCursor);
      break;
    case UBStylusTool::Line:
      controlViewport->setCursor (UBResources::resources ()->penCursor);
      break;
    case UBStylusTool::Text:
      controlViewport->setCursor (UBResources::resources ()->textCursor);
      break;
    case UBStylusTool::RichText:
      controlViewport->setCursor (UBResources::resources ()->richTextCursor);
      break;
    case UBStylusTool::Capture:
      controlViewport->setCursor (UBResources::resources ()->penCursor);
      break;
    default:
      //Q_ASSERT (false);
      //failsafe
      controlViewport->setCursor (UBResources::resources ()->penCursor);
    }
}


bool UBBoardView::hasSelectedParents(QGraphicsItem * item)
{
    if (item->isSelected())
        return true;
    if (item->parentItem()==NULL)
        return false;
    return hasSelectedParents(item->parentItem());
}
