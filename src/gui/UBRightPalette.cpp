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



#include "core/UBApplication.h"
#include "board/UBBoardController.h"

#include "UBRightPalette.h"

#include "core/memcheck.h"

/**
 * \brief The constructor
 */
UBRightPalette::UBRightPalette(QWidget *parent, const char *name):
    UBDockPalette(eUBDockPaletteType_RIGHT, parent)
{
    setObjectName(name);
    setOrientation(eUBDockOrientation_Right);
    mCollapseWidth = 150;
    bool isCollapsed = false;
    if(mCurrentMode == eUBDockPaletteWidget_BOARD){
        mLastWidth = UBSettings::settings()->rightLibPaletteBoardModeWidth->get().toInt();
        isCollapsed = UBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else if(mCurrentMode == eUBDockPaletteWidget_DESKTOP){
        mLastWidth = UBSettings::settings()->rightLibPaletteDesktopModeWidth->get().toInt();
        isCollapsed = UBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool();
    }
    else if(mCurrentMode == eUBDockPaletteWidget_WEB){
        mLastWidth = UBSettings::settings()->rightLibPaletteWebModeWidth->get().toInt();
        isCollapsed = UBSettings::settings()->rightLibPaletteWebModeIsCollapsed->get().toBool();
    }
    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(mLastWidth, parentWidget()->height());
}

/**
 * \brief The destructor
 */
UBRightPalette::~UBRightPalette()
{
}

/**
 * \brief Handle the mouse move event
 * @event as the mouse move event
 */
void UBRightPalette::mouseMoveEvent(QMouseEvent *event)
{
    if(mCanResize)
    {
        UBDockPalette::mouseMoveEvent(event);
    }
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void UBRightPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(mCurrentMode == eUBDockPaletteWidget_BOARD){
        if(newWidth > mCollapseWidth)
            UBSettings::settings()->rightLibPaletteBoardModeWidth->set(newWidth);
        UBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else if (mCurrentMode == eUBDockPaletteWidget_DESKTOP){
        if(newWidth > mCollapseWidth)
            UBSettings::settings()->rightLibPaletteDesktopModeWidth->set(newWidth);
        UBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    else if (mCurrentMode == eUBDockPaletteWidget_WEB){
        if(newWidth > mCollapseWidth)
            UBSettings::settings()->rightLibPaletteWebModeWidth->set(newWidth);
        UBSettings::settings()->rightLibPaletteWebModeIsCollapsed->set(newWidth == 0);
    }
    UBDockPalette::resizeEvent(event);
    emit resized();
}

/**
 * \brief Update the maximum width
 */
void UBRightPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
    setMaximumHeight(parentWidget()->height());
    setMinimumHeight(parentWidget()->height());
}

bool UBRightPalette::switchMode(eUBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eUBDockPaletteWidget_BOARD){
        mLastWidth = UBSettings::settings()->rightLibPaletteBoardModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(UBSettings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else if (mode == eUBDockPaletteWidget_DESKTOP){
        mLastWidth = UBSettings::settings()->rightLibPaletteDesktopModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(UBSettings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else if (mode == eUBDockPaletteWidget_WEB){
        mLastWidth = UBSettings::settings()->rightLibPaletteWebModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(UBSettings::settings()->rightLibPaletteWebModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    // HACK to force the reoganization of tabs
    if(newModeWidth == 0)
        resize(1,height());
    resize(newModeWidth,height());
    return UBDockPalette::switchMode(mode);
}


void UBRightPalette::showEvent(QShowEvent* event)
{
    UBDockPalette::showEvent(event);
}

void UBRightPalette::hideEvent(QHideEvent* event)
{
    UBDockPalette::hideEvent(event);
}
