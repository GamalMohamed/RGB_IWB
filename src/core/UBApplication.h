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



#ifndef UBAPPLICATION_H_
#define UBAPPLICATION_H_

#include <QtGui>

#include "qtsingleapplication.h"

#include "transition/UniboardSankoreTransition.h"

namespace Ui
{
    class MainWindow;
}


class UBBoardController;
class UBWebController;
class UBControlView;
class UBPreferencesController;
class UBResources;
class UBSettings;
class UBPersistenceManager;
class UBApplicationController;
class UBDocumentController;
class UBMainWindow;

class UBApplication : public QtSingleApplication
{
    Q_OBJECT

    public:

        UBApplication(const QString &id, int &argc, char **argv);
        virtual ~UBApplication();

        int exec(const QString& pFileToImport);

        void cleanup();

        static QPointer<QUndoStack> undoStack;

        static UBApplicationController *applicationController;
        static UBBoardController* boardController;
        static UBWebController* webController;
        static UBDocumentController* documentController;
        static UniboardSankoreTransition* mUniboardSankoreTransition;

        static UBMainWindow* mainWindow;

        static UBApplication* app()
        {
            return static_cast<UBApplication*>qApp;
        }

        static const QString mimeTypeUniboardDocument;
        static const QString mimeTypeUniboardPage;
        static const QString mimeTypeUniboardPageItem;
        static const QString mimeTypeUniboardPageThumbnail;

        static void showMessage(const QString& message, bool showSpinningWheel = false);
        static void setDisabled(bool disable);

        static QObject* staticMemoryCleaner;

        void decorateActionMenu(QAction* action);
        void insertSpaceToToolbarBeforeAction(QToolBar* toolbar, QAction* action, int width = -1);

        int toolBarHeight();
        bool eventFilter(QObject *obj, QEvent *event);

        bool isVerbose() { return mIsVerbose;}
        void setVerbose(bool verbose){mIsVerbose = verbose;}
        static void loadUrl(const QString& url);
        static QString urlFromHtml(QString html);
        static bool isFromWeb(QString url);
//        bool notify ( QObject * receiver, QEvent * event );

    signals:

    public slots:

        void showBoard();
        void showInternet();
        void showDocument();
        void startScript();
        void stopScript();

        void toolBarPositionChanged(QVariant topOrBottom);
        void toolBarDisplayTextChanged(QVariant display);

        void closeEvent(QCloseEvent *event);

        /**
         * Used on Windows platform to open file in running application. On MacOS X opening file is done through the
         * FileOpen event that is handle in eventFilter method.
         */
        bool handleOpenMessage(const QString& pMessage);

    private slots:

        void closing();
#ifdef Q_WS_MAC
        void showMinimized();
#endif
        void importUniboardFiles();

        void onScreenCountChanged(int newCount);

    private:
        void updateProtoActionsState();
        void setupTranslators(QStringList args);
        QList<QMenu*> mProtoMenus;
        bool mIsVerbose;
        QString checkLanguageAvailabilityForSankore(QString& language);
    protected:

#if defined(Q_WS_MACX) && !defined(QT_MAC_USE_COCOA)
        bool macEventFilter(EventHandlerCallRef caller, EventRef event);
#endif

        UBPreferencesController* mPreferencesController;
        QTranslator* mApplicationTranslator;
        QTranslator* mQtGuiTranslator;

};


class UBStyle : public QPlastiqueStyle
{
    public:

        UBStyle()
            : QPlastiqueStyle()
        {
            // NOOP
        }

        virtual ~UBStyle()
        {
           // NOOP
        }

        /*
         * redefined to be more cocoa like on texts
         */
        virtual void drawItemText(QPainter *painter, const QRect &rect, int alignment, const QPalette &pal,
                                  bool enabled, const QString& text, QPalette::ColorRole textRole) const;


};

#endif /* UBAPPLICATION_H_ */
