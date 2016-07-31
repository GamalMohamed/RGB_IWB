#ifndef UBMAINWINDOW_H_
#define UBMAINWINDOW_H_

#include <QMainWindow>
#include <QWidget>
#include <QWebView>
#include <QMessageBox>
#include "UBDownloadWidget.h"

class QStackedLayout;

#include "ui_mainWindow.h"

class UBMainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT
    public:

        UBMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
        virtual ~UBMainWindow();

        void addBoardWidget(QWidget *pWidget);
        void switchToBoardWidget();

        void addWebWidget(QWidget *pWidget);
        void switchToWebWidget();

        void addDocumentsWidget(QWidget *pWidget);
        void switchToDocumentsWidget();

        bool yesNoQuestion(QString windowTitle, QString text);
        void warning(QString windowTitle, QString text);
        void information(QString windowTitle, QString text);

        void showDownloadWidget();
        void hideDownloadWidget();

    signals:
        void closeEvent_Signal( QCloseEvent *event );

    public slots:
        void onExportDone();

    protected:
        void oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type);

        virtual void keyPressEvent(QKeyEvent *event);
        virtual void closeEvent (QCloseEvent *event);

        virtual QMenu* createPopupMenu ()
        {
            // no pop up on toolbar
            return 0;
        }

        QStackedLayout* mStackedLayout;

        QWidget *mBoardWidget;
        QWidget *mWebWidget;
        QWidget *mDocumentsWidget;


private:
// work around for handling tablet events on MAC OS with Qt 4.8.0 and above
#if defined(Q_WS_MACX)
        bool event(QEvent *event);
#endif
        UBDownloadWidget* mpDownloadWidget;
};

#endif /* UBMAINWINDOW_H_ */
