#ifndef UBPREFERENCESCONTROLLER_H_
#define UBPREFERENCESCONTROLLER_H_

#include <QtGui>

class UBColorPicker;
class UBApplication;
class UBSettings;
class UBPreferencesController;

namespace Ui
{
    class preferencesDialog;
}

#include "ui_brushProperties.h"

class UBBrushPropertiesFrame;


class UBPreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    UBPreferencesDialog(UBPreferencesController* prefController, QWidget* parent = 0,Qt::WindowFlags f = 0 );
    ~UBPreferencesDialog();

protected:
    void closeEvent(QCloseEvent* e);
    UBPreferencesController *mPreferencesController;
};


class UBPreferencesController : public QObject
{
    Q_OBJECT

    public:
        UBPreferencesController(QWidget *parent);
        virtual ~UBPreferencesController();

        bool inputValuesConsistence();

    public slots:

        void show();

    protected:

        void wire();
        void init();

        QMap<QString,QString>mIsoCodeAndLanguage;

        UBPreferencesDialog* mPreferencesWindow;
        Ui::preferencesDialog* mPreferencesUI;
        UBBrushPropertiesFrame* mPenProperties;
        UBBrushPropertiesFrame* mMarkerProperties;

    protected slots:

        void close();
        void defaultSettings();
        void widthSliderChanged(int value);
        void opacitySliderChanged(int value);
        void colorSelected(const QColor&);
        void toolbarPositionChanged(bool checked);
        void toolbarOrientationVertical(bool checked);
        void toolbarOrientationHorizontal(bool checked);
        void onCommunityUsernameChanged();
        void onCommunityPasswordChanged();
        void onCommunityPersistenceChanged();
        void onLanguageChanged(QString currentItem);
        void onStartupTipsClicked(bool clicked);

    private slots:
        void adjustScreens(int screen);

    private:
        static qreal sSliderRatio;
        static qreal sMinPenWidth;
        static qreal sMaxPenWidth;
        void persistanceCheckboxUpdate();
        QDesktopWidget* mDesktop;

};

class UBBrushPropertiesFrame : public Ui::brushProperties
{

    public:
        UBBrushPropertiesFrame(QFrame* owner, const QList<QColor>& lightBackgroundColors,const QList<QColor>& darkBackgroundColors, const QList<QColor>& lightBackgroundSelectedColors,const QList<QColor>& darkBackgroundSelectedColors, UBPreferencesController* controller);

        virtual ~UBBrushPropertiesFrame(){}

        QList<UBColorPicker*> lightBackgroundColorPickers;
        QList<UBColorPicker*> darkBackgroundColorPickers;

};


#endif /* UBPREFERENCESCONTROLLER_H_ */
