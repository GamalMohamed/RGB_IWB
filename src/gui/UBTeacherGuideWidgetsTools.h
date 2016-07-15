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



#ifndef UBTEACHERGUIDEWIDGETSTOOLS_H
#define UBTEACHERGUIDEWIDGETSTOOLS_H

#include <QObject>
#include <QTreeWidgetItem>
#include <QTextEdit>
#include <QLabel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLineEdit>
#include <QMimeData>
#include <QStackedWidget>
#include <QWebView>
#include <QFocusEvent>
#include <QMouseEvent>

#include "customWidgets/UBMediaWidget.h"

#define TG_USER_ROLE_MIME_TYPE (Qt::UserRole+50)


class QTreeWidget;
class QVBoxLayout;
class QComboBox;
class QTextEdit;
class QWidget;
class UBTGAdaptableText;
class QDomElement;
class UBMediaWidget;

typedef struct
{
    QString name;
    QMap<QString,QString> attributes;
}tUBGEElementNode;


class iUBTGSaveData
{
public:
    virtual tUBGEElementNode* saveData() = 0;
};


class UBAddItem : public QTreeWidgetItem
{
public:
    explicit UBAddItem(const QString &strings, int addSubItemWidgetType, QTreeWidget* parent = 0);
    ~UBAddItem();

signals:

public slots:
};

class UBTGActionWidget : public QWidget, public iUBTGSaveData
{
    Q_OBJECT

public:
    explicit UBTGActionWidget(QTreeWidgetItem* widget, QWidget* parent = 0,const char* name = "UBTGActionWidget");

    UBTGActionWidget(const UBTGActionWidget& src);

    ~UBTGActionWidget();
    void update();
    tUBGEElementNode* saveData();
    void initializeWithDom(QDomElement element);

//N/C - NNE - 20140328 : Sort the actions
public slots:
    void onUpButton();
    void onDownButton();
    void onClose();
//N/C - NNE - 20140328 : END

//N/C - NNE - 20141020
    void onChange();
//N/C - NNE - 20141020 : END

signals:
    void hasChanged();

private:
    QVBoxLayout* mpLayout;
    QComboBox* mpOwner;
    UBTGAdaptableText* mpTask;

protected:
    QTreeWidgetItem* mpTreeWidgetItem;
};


//N/C - NNE - 20140331
class UBTGActionColumn : public QWidget
{
    Q_OBJECT

public:
    UBTGActionColumn(QTreeWidgetItem *widgetItem, QWidget *parent = 0);

signals:
    void clickOnClose(QTreeWidgetItem*);
    void clickOnUp(QTreeWidgetItem*);
    void clickOnDown(QTreeWidgetItem*);

    void clickOnClose();
    void clickOnUp();
    void clickOnDown();

private slots:
    void onClickClose();
    void onClickUp();
    void onClickDown();

private:
    QPushButton *mCloseButton;
    QPushButton *mUpButton;
    QPushButton *mDownButton;

    QTreeWidgetItem *mWidgetItem;
};

//N/C - NNE - 20140331 : END

class UBTGAdaptableText : public QTextEdit
{
    Q_OBJECT
public:
    explicit UBTGAdaptableText(QTreeWidgetItem* widget = 0, QWidget *parent = 0, const char* name = "UBTGAdaptableText");
    void showText(const QString & text);
    void resetText();
    void bottomMargin(int newValue);
    void setPlaceHolderText(QString text);
    QString text();
    void setInitialText(const QString& text);
    void setMaximumLength(int length);
    void managePlaceholder(bool focus);

public slots:
    void onTextChanged();

protected:
    void keyReleaseEvent(QKeyEvent* e);
    void showEvent(QShowEvent* e);
    void focusInEvent(QFocusEvent* e);
    void focusOutEvent(QFocusEvent* e);
    virtual void insertFromMimeData(const QMimeData *source);

private:
    void setCursorToTheEnd();
    int mBottomMargin;
    QTreeWidgetItem* mpTreeWidgetItem;
    int mMinimumHeight;
    bool mHasPlaceHolder;
    QString mPlaceHolderText;
    bool mIsUpdatingSize;
    int mMaximumLength;
};


class UBDraggableWeb : public QWebView
{
    Q_OBJECT
public:
    explicit UBDraggableWeb(QString& relativePath, QWidget* parent = 0);

private:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

    QString mRelativePath;
    QPoint mDragStartPosition;
    bool mDragStarted;
};

class UBTGMediaWidget : public QStackedWidget , public iUBTGSaveData
{
    Q_OBJECT
public:
    UBTGMediaWidget(QTreeWidgetItem* widget = 0, QWidget* parent = 0, const char* name = "UBTGMediaWidget");
    UBTGMediaWidget(QString mediaPath, QTreeWidgetItem* widget = 0, QWidget* parent = 0, bool forceFlashMediaType = false, const char *name = "UBTGMediaWidget");
    ~UBTGMediaWidget();
    tUBGEElementNode* saveData();
    void initializeWithDom(QDomElement element);
    void removeSource();

    //N/C - NNE - 20140401
    UBTGMediaWidget *clone() const;
public slots:
    void onUpButton();
    void onDownButton();
    void onClose();
    void onTitleChanged();

protected:
    void dragEnterEvent(QDragEnterEvent* event);
    void dropEvent(QDropEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);

private:
    void parseMimeData(const QMimeData* pMimeData);
    void createWorkWidget(bool forceFlashMediaType = false);
    void updateSize();

    QTreeWidgetItem* mpTreeWidgetItem;
    QLabel* mpDropMeWidget;
    QWidget* mpWorkWidget;
    QVBoxLayout* mpLayout;
    QHBoxLayout* mpMediaLayout;
    UBTGAdaptableText* mpTitle;
    QLabel* mpMediaLabelWidget;
    UBMediaWidget* mpMediaWidget;
    UBDraggableWeb* mpWebView;
    QString mMediaPath;
    bool mIsPresentationMode;
    QString mMediaType;
    bool mIsInitializationMode;
    int mMediaWidgetHeight;
};

class UBTGUrlWidget : public QWidget , public iUBTGSaveData
{
    Q_OBJECT
public:
    UBTGUrlWidget(QTreeWidgetItem *treeWidgetItem, QWidget* parent = 0, const char* name = "UBTGUrlWidget");
    ~UBTGUrlWidget();
    tUBGEElementNode* saveData();
    void initializeWithDom(QDomElement element);

    UBTGUrlWidget *clone() const;

public slots:
    void onUrlEditionFinished();
    void onDownButton();
    void onClose();
    void onUpButton();

private:
    QVBoxLayout* mpLayout;
    QLineEdit* mpTitle;
    QLineEdit* mpUrl;

    //N/C - NNE - 20140401
    QTreeWidgetItem *mTreeWidgetItem;
};

class UBTGDraggableTreeItem : public QTreeWidget
{
    Q_OBJECT
public:
    UBTGDraggableTreeItem(QWidget* parent = 0, const char* name = "UBTGDraggableTreeItem");
private:
    QMimeData* mimeData(const QList<QTreeWidgetItem *> items) const;
};

// Issue 1683 (Evolution) - AOU - 20131206
class UBTGFileWidget : public QWidget, public iUBTGSaveData
{
    Q_OBJECT
public:
    UBTGFileWidget(QTreeWidgetItem *treeWidgetItem , QWidget *parent = 0, const char *name = "UBTGFileWidget");
    virtual ~UBTGFileWidget();
    tUBGEElementNode* saveData();
    void initializeWithDom(QDomElement element);
    inline QString titreFichier() const {return mpTitreFichier->text();}
    inline QString path() const {return mPath;}
    inline void setPath(QString path){mPath = path;}
    UBTGFileWidget * clone() const;

private:
    QVBoxLayout * mpLayout;
    QHBoxLayout * mpHLayout;
    QLineEdit * mpTitreFichier;
    QLineEdit * mpNomFichier;
    QPushButton * mpBtnSelectFile;
    QString mPath;

    QTreeWidgetItem *mTreeWidgetItem;

private slots:
    void OnClickBtnSelectFile();

public slots:
    void onUpButton();
    void onDownButton();
    void onClose();


signals:
    void changed();


};
// Fin Issue 1683 (Evolution) - AOU - 20131206

#endif // UBTEACHERGUIDEWIDGETSTOOLS_H
