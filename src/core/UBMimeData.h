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



#ifndef UBMIMEDATA_H_
#define UBMIMEDATA_H_

#include <QtGui>

class UBDocumentProxy;
class UBItem;

struct UBMimeDataItem
{
    public:
        UBMimeDataItem(UBDocumentProxy* proxy, int sceneIndex, UBDocumentProxy *targetProxy = 0);
        virtual ~UBMimeDataItem();

        UBDocumentProxy* documentProxy() const { return mProxy; }
        int sceneIndex() const { return mSceneIndex; }


    private:
        UBDocumentProxy* mProxy;
        int mSceneIndex;
        UBDocumentProxy *mTargetProxy;
};


class UBMimeDataGraphicsItem : public QMimeData
{
    Q_OBJECT;

    public:
            UBMimeDataGraphicsItem(QList<UBItem*> pItems);
        virtual ~UBMimeDataGraphicsItem();

        QList<UBItem*> items() const { return mItems; }

    private:
        QList<UBItem*> mItems;

};




class UBMimeData : public QMimeData
{
    Q_OBJECT;

    public:
        UBMimeData(const QList<UBMimeDataItem> &items);
        virtual ~UBMimeData();

        QList<UBMimeDataItem> items() const { return mItems; }

    private:
        QList<UBMimeDataItem> mItems;
};

#endif /* UBMIMEDATA_H_ */
