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



#ifndef UBGRAPHICSMEDIAITEMDELEGATE_H_
#define UBGRAPHICSMEDIAITEMDELEGATE_H_

#include <QtGui>
#include <phonon/MediaObject>
#include <QTimer>

#include "core/UB.h"
#include "UBGraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;

class UBGraphicsMediaItemDelegate :  public UBGraphicsItemDelegate
{
    Q_OBJECT

    public:
        UBGraphicsMediaItemDelegate(UBGraphicsMediaItem* pDelegated, Phonon::MediaObject* pMedia, QObject * parent = 0);
        virtual ~UBGraphicsMediaItemDelegate();

        virtual void positionHandles();

        bool mousePressEvent(QGraphicsSceneMouseEvent *event);

    public slots:

        void toggleMute();
        void updateTicker(qint64 time);

    protected slots:

        virtual void remove(bool canUndo = true);

        void togglePlayPause();

        void mediaStateChanged ( Phonon::State newstate, Phonon::State oldstate );

        void updatePlayPauseState();

        void totalTimeChanged(qint64 newTotalTime);

        void hideToolBar();

    protected:
        virtual void buildButtons();

        UBGraphicsMediaItem* delegated();

        DelegateButton* mPlayPauseButton;
        DelegateButton* mStopButton;
        DelegateButton* mMuteButton;
        DelegateMediaControl *mMediaControl;

        Phonon::MediaObject* mMedia;

        QTimer *mToolBarShowTimer;
        int m_iToolBarShowingInterval;
};

#endif /* UBGRAPHICSMEDIAITEMDELEGATE_H_ */