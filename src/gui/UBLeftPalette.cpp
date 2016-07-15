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



#include "UBLeftPalette.h"
#include "core/UBSettings.h"

#include "core/memcheck.h"

/**
 * \brief The constructor
 */
UBLeftPalette::UBLeftPalette(QWidget *parent, const char *name):
    UBDockPalette(eUBDockPaletteType_LEFT, parent)
{
    setObjectName(name);
    setOrientation(eUBDockOrientation_Left);
    mCollapseWidth = 150;

    bool isCollapsed = false;
    if(mCurrentMode == eUBDockPaletteWidget_BOARD){
        mLastWidth = UBSettings::settings()->leftLibPaletteBoardModeWidth->get().toInt();
        isCollapsed = UBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else{
        mLastWidth = UBSettings::settings()->leftLibPaletteDesktopModeWidth->get().toInt();
        isCollapsed = UBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool();
    }

    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(mLastWidth, parentWidget()->height());
}

/**
 * \brief The destructor
 */
UBLeftPalette::~UBLeftPalette()
{

}


void UBLeftPalette::onDocumentSet(UBDocumentProxy* documentProxy)
{
    //This is necessary to force the teacher guide to be showed in priority each time a document is set
    if(documentProxy && UBSettings::settings()->teacherGuidePageZeroActivated->get().toBool())
        mLastOpenedTabForMode.insert(eUBDockPaletteWidget_BOARD, 1);
}

/**
 * \brief Update the maximum width
 */
void UBLeftPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void UBLeftPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(mCurrentMode == eUBDockPaletteWidget_BOARD){
        if(newWidth > mCollapseWidth)
            UBSettings::settings()->leftLibPaletteBoardModeWidth->set(newWidth+1);
        UBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else if (mCurrentMode == eUBDockPaletteWidget_DESKTOP){
        if(newWidth > mCollapseWidth)
            UBSettings::settings()->leftLibPaletteDesktopModeWidth->set(newWidth);
        UBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    UBDockPalette::resizeEvent(event);
}


bool UBLeftPalette::switchMode(eUBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eUBDockPaletteWidget_BOARD){
        mLastWidth = UBSettings::settings()->leftLibPaletteBoardModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(UBSettings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else if (mode == eUBDockPaletteWidget_DESKTOP){
        mLastWidth = UBSettings::settings()->leftLibPaletteDesktopModeWidth->get().toInt();
        newModeWidth = mLastWidth;
        if(UBSettings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    //TODO claudio another hack
    // avoid the overlap of tab and menu bar (positionned on bottom) when clicling between
    // board and web mode
    resize(newModeWidth == 0 ? 1 : newModeWidth,height());
    return UBDockPalette::switchMode(mode);
}
