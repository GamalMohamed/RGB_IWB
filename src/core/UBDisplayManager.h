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
