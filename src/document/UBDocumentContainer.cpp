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


#include "UBDocumentContainer.h"
#include "adaptors/UBThumbnailAdaptor.h"

#include "core/UBPersistenceManager.h"
#include "core/memcheck.h"
#include "core/UBApplication.h"
#include "core/UBApplicationController.h"

#include "frameworks/UBFileSystemUtils.h"

UBDocumentContainer::UBDocumentContainer(QObject * parent)
    :QObject(parent)
    ,mCurrentDocument(NULL)
{}

UBDocumentContainer::~UBDocumentContainer()
{
    if (mDocumentThumbs.size() > 0){
        foreach(const QPixmap* pm, mDocumentThumbs){
            delete pm;
            pm = NULL;
        }
    }
}

void UBDocumentContainer::setDocument(UBDocumentProxy* document, bool forceReload)
{
    if (mCurrentDocument != document || forceReload)
    {
        mCurrentDocument = document;
        reloadThumbnails();
        emit documentSet(mCurrentDocument);
    }
}

void UBDocumentContainer::duplicatePages(QList<int>& pageIndexes)
{
    int offset = 0;
    foreach(int sceneIndex, pageIndexes) {
//        UBPersistenceManager::persistenceManager()->duplicateDocumentScene(mCurrentDocument, sceneIndex + offset);
        UBPersistenceManager::persistenceManager()->copyDocumentScene(mCurrentDocument, sceneIndex + offset,
                                                                      mCurrentDocument, sceneIndex + offset + 1);
        offset++;
    }
}

bool UBDocumentContainer::movePageToIndex(int source, int target)
{
    if (source==0)
    {
        // Title page - cant be moved
        return false;
    }
    UBPersistenceManager::persistenceManager()->moveSceneToIndex(mCurrentDocument, source, target);
    deleteThumbPage(source);
    insertThumbPage(target);
    emit documentThumbnailsUpdated(this);
    return true;
}

void UBDocumentContainer::deletePages(QList<int>& pageIndexes)
{
    UBPersistenceManager::persistenceManager()->deleteDocumentScenes(mCurrentDocument, pageIndexes);
    int offset = 0;
    foreach(int index, pageIndexes)
    {
        deleteThumbPage(index - offset);
        offset++;
    }
    emit documentThumbnailsUpdated(this);
}

void UBDocumentContainer::addPage(int index)
{
    UBPersistenceManager::persistenceManager()->createDocumentSceneAt(mCurrentDocument, index);
    insertThumbPage(index);
    emit documentThumbnailsUpdated(this);
}

void UBDocumentContainer::updatePage(int index)
{
    updateThumbPage(index);
    emit documentThumbnailsUpdated(this);
}

void UBDocumentContainer::deleteThumbPage(int index)
{
    mDocumentThumbs.removeAt(index);
}

void UBDocumentContainer::updateThumbPage(int index)
{

    //bad hack for last page duplicated + action that lead to a crash
    if(index < mDocumentThumbs.count()){
        mDocumentThumbs[index] = UBThumbnailAdaptor::get(mCurrentDocument, index);
        emit documentPageUpdated(index);
    }
}

void UBDocumentContainer::insertThumbPage(int index)
{
    mDocumentThumbs.insert(index, UBThumbnailAdaptor::get(mCurrentDocument, index));
}

void UBDocumentContainer::reloadThumbnails()
{
    if (mCurrentDocument) {        
        if (mDocumentThumbs.size() > 0)
            UBThumbnailAdaptor::clearThumbs(mDocumentThumbs);
        UBThumbnailAdaptor::load(mCurrentDocument, mDocumentThumbs);
    } else {
        if (mDocumentThumbs.size() > 0)
            UBThumbnailAdaptor::clearThumbs(mDocumentThumbs);
    }
    emit documentThumbnailsUpdated(this);
}

void UBDocumentContainer::addPixmapAt(const QPixmap *pix, int index)
{
    mDocumentThumbs.insert(index, pix);
    emit documentThumbnailsUpdated(this);
}

int UBDocumentContainer::pageFromSceneIndex(int sceneIndex)
{
    if(UBSettings::settings()->teacherGuidePageZeroActivated->get().toBool())
        return sceneIndex;
    return sceneIndex+1;
}

int UBDocumentContainer::sceneIndexFromPage(int page)
{
    if(UBSettings::settings()->teacherGuidePageZeroActivated->get().toBool())
        return page;
    return page-1;
}

const QPixmap* UBDocumentContainer::pageAt(int index)
{
    return mDocumentThumbs[index];
}
