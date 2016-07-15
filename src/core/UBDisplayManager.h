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



#ifndef UBDISPLAYMANAGER_H_
#define UBDISPLAYMANAGER_H_

#include <QtGui>

class UBBlackoutWidget;
class UBBoardView;

class UBDisplayManager : public QObject
{
    Q_OBJECT;

    public:
        UBDisplayManager(QObject *parent = 0);
        virtual ~UBDisplayManager();

        int numScreens();

        int numPreviousViews();

        void setControlWidget(QWidget* pControlWidget);

        void setDisplayWidget(QWidget* pDisplayWidget);

        void setDesktopWidget(QWidget* pControlWidget);

        void setPreviousDisplaysWidgets(QList<UBBoardView*> pPreviousViews);

        bool hasControl()
        {
            return mControlScreenIndex > -1;
        }

        bool hasDisplay()
        {
            return mDisplayScreenIndex > -1;
        }

        bool hasPrevious()
        {
            return !mPreviousScreenIndexes.isEmpty();
        }

        enum DisplayRole
        {
            None = 0, Control, Display, Previous1, Previous2, Previous3, Previous4, Previous5
        };

        void setUseMultiScreen(bool pUse);

        int controleScreenIndex()
        {
            return mControlScreenIndex;
        }

        QRect controlGeometry();
        QRect displayGeometry();

   signals:

           void screenLayoutChanged();

   public slots:

        void reinitScreens(bool bswap);

        void adjustScreens(int screen);

        void blackout();

        void unBlackout();

        void setRoleToScreen(DisplayRole role, int screenIndex);

    private:

        void positionScreens();

        void initScreenIndexes();

        int mControlScreenIndex;

        int mDisplayScreenIndex;

        QList<int> mPreviousScreenIndexes;

        QDesktopWidget* mDesktop;

        QWidget* mControlWidget;

        QWidget* mDisplayWidget;

        QWidget *mDesktopWidget;

        QList<UBBoardView*> mPreviousDisplayWidgets;

        QList<UBBlackoutWidget*> mBlackoutWidgets;

        QList<DisplayRole> mScreenIndexesRoles;

        bool mUseMultiScreen;

};

#endif /* UBDISPLAYMANAGER_H_ */
