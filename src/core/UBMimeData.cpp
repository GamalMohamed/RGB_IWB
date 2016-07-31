#include "UBMimeData.h"

#include <QtGui>

#include "core/UBApplication.h"
#include "domain/UBItem.h"

#include "core/memcheck.h"

UBMimeDataItem::UBMimeDataItem(UBDocumentProxy* proxy, int sceneIndex, UBDocumentProxy *targetProxy)
    : mProxy(proxy)
    , mSceneIndex(sceneIndex)
    , mTargetProxy(targetProxy)
{
    // NOOP
}

UBMimeDataItem::~UBMimeDataItem()
{
    // NOOP
}

UBMimeData::UBMimeData(const QList<UBMimeDataItem> &items)
    : QMimeData()
    , mItems(items)
{
    setData(UBApplication::mimeTypeUniboardPage, QByteArray());
}

UBMimeData::~UBMimeData()
{
    // NOOP
}

UBMimeDataGraphicsItem::UBMimeDataGraphicsItem(QList<UBItem*> pItems)
{
        mItems = pItems;
}

UBMimeDataGraphicsItem::~UBMimeDataGraphicsItem()
{
}
