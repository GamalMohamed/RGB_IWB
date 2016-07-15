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



#ifndef UBDOCKPALETTEWIDGET_H
#define UBDOCKPALETTEWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QString>


/**
  * This enum defines the different mode availables.
  */
typedef enum
{
    eUBDockPaletteWidget_BOARD,
    eUBDockPaletteWidget_WEB,
    eUBDockPaletteWidget_DOCUMENT,
    eUBDockPaletteWidget_DESKTOP
} eUBDockPaletteWidgetMode;

class UBDockPaletteWidget : public QWidget
{

    Q_OBJECT
public:
    UBDockPaletteWidget(QWidget* parent=0, const char* name="UBDockPaletteWidget");
    ~UBDockPaletteWidget();

    virtual QPixmap iconToRight() const;
    virtual QPixmap iconToLeft() const;
    QString name();

    //issue 1682 - NNE - 20140113 : Add the currentPage parameter
    virtual bool visibleInMode(eUBDockPaletteWidgetMode mode, int currentPage) = 0;

    void registerMode(eUBDockPaletteWidgetMode mode);

    bool visibleState(){return mVisibleState;}
    void setVisibleState(bool state){mVisibleState = state;}

signals:
    void hideTab(UBDockPaletteWidget* widget);
    void showTab(UBDockPaletteWidget* widget);

public slots:
    void slot_changeMode(eUBDockPaletteWidgetMode newMode);


protected:
//    QPixmap mIconToRight;   // arrow like this: >
//    QPixmap mIconToLeft;    // arrow like this: <
    QString mName;

    /* The current widget available mode list */
    QVector<eUBDockPaletteWidgetMode> mRegisteredModes;

    bool mVisibleState;
};

#endif // UBDOCKPALETTEWIDGET_H
