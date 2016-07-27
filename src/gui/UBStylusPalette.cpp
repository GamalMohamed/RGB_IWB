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
#include "UBStylusPalette.h"

#include <QtGui>

#include "UBMainWindow.h"

#include "core/UBApplication.h"
#include "core/UBSettings.h"
#include "core/UBApplicationController.h"

#include "board/UBDrawingController.h"

#include "frameworks/UBPlatformUtils.h"

#include "core/memcheck.h"

UBStylusPalette::UBStylusPalette(QWidget *parent, Qt::Orientation orient)
    : UBActionPalette(Qt::TopRightCorner, parent, orient)
    , mLastSelectedId(-1)
{
    QList<QAction*> actions;

    actions << UBApplication::mainWindow->actionDrawing;
    actions << UBApplication::mainWindow->actionSelector;

    actions << UBApplication::mainWindow->actionPen;
    actions << UBApplication::mainWindow->actionEraser;
    actions << UBApplication::mainWindow->actionMarker;
    actions << UBApplication::mainWindow->actionLine; // ALTI/AOU - 20140606 : restore Line tool

    actions << UBApplication::mainWindow->actionPlay;
    actions << UBApplication::mainWindow->actionHand;
    actions << UBApplication::mainWindow->actionZoomIn;
    actions << UBApplication::mainWindow->actionZoomOut;
    actions << UBApplication::mainWindow->actionPointer;
    actions << UBApplication::mainWindow->actionText;
    actions << UBApplication::mainWindow->actionCapture;

    /* ALTI/AOU - 20140606 : RichTextEditor tool isn't available anymore.
    UBApplication::mainWindow->actionRichTextEditor->setEnabled(UBFeaturesController::RTEIsLoaded());
    actions << UBApplication::mainWindow->actionRichTextEditor;
    */

    if(UBPlatformUtils::hasVirtualKeyboard())
        actions << UBApplication::mainWindow->actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(39, 39));

    if(!UBPlatformUtils::hasVirtualKeyboard())
    {
        groupActions();
    }
    else
    {
        // VirtualKeyboard and Drawing actions are not in group
        // So, groupping all buttons, except first and last
        mButtonGroup = new QButtonGroup(this);
        for(int i=1; i < mButtons.size()-1; i++)
        {
            mButtonGroup->addButton(mButtons[i], i);
        }
        connect(mButtonGroup, SIGNAL(buttonClicked(int)), this, SIGNAL(buttonGroupClicked(int)));
    }

    adjustSizeAndPosition();
    initPosition();
    setBackgroundColor();

    foreach(const UBActionPaletteButton* button, mButtons)
    {
        connect(button, SIGNAL(doubleClicked()), this, SLOT(stylusToolDoubleClicked()));
    }
}


UBStylusPalette::~UBStylusPalette()
{

}

void UBStylusPalette::initPosition()
{
    QWidget* pParentW = parentWidget();

    if(NULL != pParentW)
    {
        QPoint pos;
        int parentWidth = pParentW->width();
        int parentHeight = pParentW->height();
        int posX,posY;
        mCustomPosition = true;

        if(!UBSettings::settings()->appToolBarOrientationVertical->get().toBool())
        {
                posX = (parentWidth / 2) - (width() / 2);
                posY=0;
                //int posY = parentHeight - border() - (parentHeight-height());
        }
        else
        {
                posX = parentWidth;
                posY = (parentHeight / 2) - (height() / 2);
        }

        pos.setX(posX);
        pos.setY(posY);
        moveInsideParent(pos);
    }
}

void UBStylusPalette::setBackgroundColor()
{
    QBrush *brush = new QBrush();
    brush->setStyle(Qt::SolidPattern);
    brush->setColor(QColor(85, 170, 255));
    setBackgroundBrush(*brush);
}

void UBStylusPalette::stylusToolDoubleClicked()
{
    //Issue "retours 2.4RC1 - CFA - 20140217 : la drawingPalette créé un décalage d'index entre checkedId et les valeurs correspondantes dans l'enum UBStylusTool
    //on décrémente pour rétablir la correspondance
    emit stylusToolDoubleClicked(mButtonGroup->checkedId() -1 );
}


