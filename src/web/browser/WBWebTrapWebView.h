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



#ifndef WBWEBTRAPWEBVIEW_H_
#define WBWEBTRAPWEBVIEW_H_

#include <QtGui>
#include <QtWebKit>


class WBWebTrapWebView : public QWebView
{
    Q_OBJECT

    public:
        WBWebTrapWebView(QWidget* parent = 0);
        virtual ~WBWebTrapWebView();

        void setIsTrapping(bool pIsTrapping);

        bool isTrapping()
        {
            return mIsTrapping;
        }

        void highliteElementAtPos(const QPoint& pos);
        void trapContentFromHitTest(const QWebHitTestResult& hitTest);
        void trapElementAtPos(const QPoint& pos);

    signals:

        void pixmapCaptured(const QPixmap& pixmap, bool pageMode);
        void objectCaptured(const QUrl& pUrl, const QString& pMimeType, int pWidth, int pHeight);
        void embedCodeCaptured(const QString& embedCode);

        void webElementCaptured(const QUrl& pUrl, const QString& query);

    private:
        bool isFrameContentAtPos(const QPoint &point) const;

    protected:
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void hideEvent ( QHideEvent * event );

        virtual void paintEvent ( QPaintEvent * event );

        QString potentialEmbedCodeAtPos(const QPoint& pos);

        enum WebContentType
        {
            Unknown = 0, Image, Input, ObjectOrEmbed, ElementByQuery
        };

    private:
        QRect mDomElementRect;
        QRect mWebViewElementRect;
        QString mElementQuery;
        WebContentType mCurrentContentType;

        bool mIsTrapping;
        bool m_bContentForTrapSelected;
        bool m_bScrollingInProcess;

    private slots:
        void viewLoadFinished(bool ok);
};
class UBTrapWebPageContentController;
class UBWebTrapMouseEventMask : public QWidget
{
    public:
        UBWebTrapMouseEventMask(WBWebTrapWebView* pWebView);
        UBWebTrapMouseEventMask(WBWebTrapWebView* pWebView, UBTrapWebPageContentController *controller);
        virtual ~UBWebTrapMouseEventMask();

    protected:

        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseMoveEvent ( QMouseEvent * event );
        virtual void mouseReleaseEvent ( QMouseEvent * event );
        virtual void paintEvent(QPaintEvent *);

   private:
       WBWebTrapWebView *mTrappedWebView;
       UBTrapWebPageContentController *mTrapController;
};

#endif /* WBWEBTRAPWEBVIEW_H_ */
