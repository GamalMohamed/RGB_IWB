#ifndef UBDOWNLOADWIDGET_H
#define UBDOWNLOADWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QItemDelegate>

#include "core/UBDownloadManager.h"

typedef enum{
    eItemColumn_Desc,
    eItemColumn_Close
}eItemColumn;

class UBDownloadProgressDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    UBDownloadProgressDelegate(QObject* parent=0);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class UBDownloadWidget : public QWidget
{
    Q_OBJECT
public:
    UBDownloadWidget(QWidget* parent=0, const char* name="UBDownloadWidget");
    ~UBDownloadWidget();

private slots:
    void onFileAddedToDownload();
    void onDownloadUpdated(int id, qint64 crnt, qint64 total);
    void onDownloadFinished(int id);
    void onCancelClicked();
    void onItemClicked(QTreeWidgetItem* pItem, int col);

private:
    void addCurrentDownloads();
    void addPendingDownloads();

    /** The general layout of this widget */
    QVBoxLayout* mpLayout;

    /** The button layout */
    QHBoxLayout* mpBttnLayout;

    /** The treeview that will display the files list */
    QTreeWidget* mpTree;

    /** The 'Cancel' button */
    QPushButton* mpCancelBttn;

    /** A temporary tree widget item */
    QTreeWidgetItem* mpItem;

    /** The delegate that will draw the progressbars */
    UBDownloadProgressDelegate mProgressBarDelegate;
};

#endif // UBDOWNLOADWIDGET_H
