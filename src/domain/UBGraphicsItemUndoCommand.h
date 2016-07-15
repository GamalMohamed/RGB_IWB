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



#ifndef UBGRAPHICSITEMUNDOCOMMAND_H_
#define UBGRAPHICSITEMUNDOCOMMAND_H_

#include <QtGui>
#include "UBAbstractUndoCommand.h"
#include "UBGraphicsGroupContainerItem.h"


class UBGraphicsScene;


class UBGraphicsItemUndoCommand : public UBAbstractUndoCommand
{
    public:
        typedef QMultiMap<UBGraphicsGroupContainerItem*, QUuid> GroupDataTable;

        UBGraphicsItemUndoCommand(UBGraphicsScene* pScene, const QSet<QGraphicsItem*>& pRemovedItems,
                                  const QSet<QGraphicsItem*>& pAddedItems, const GroupDataTable &groupsMap = GroupDataTable());

        UBGraphicsItemUndoCommand(UBGraphicsScene* pScene, QGraphicsItem* pRemovedItem,
                        QGraphicsItem* pAddedItem);

        virtual ~UBGraphicsItemUndoCommand();

        QSet<QGraphicsItem*> GetAddedList() const { return mAddedItems; }
        QSet<QGraphicsItem*> GetRemovedList() const { return mRemovedItems; }

        virtual UndoType getType() const { return undotype_GRAPHICITEM; }

    protected:
        virtual void undo();
        virtual void redo();

    private:
        UBGraphicsScene* mScene;
        QSet<QGraphicsItem*> mRemovedItems;
        QSet<QGraphicsItem*> mAddedItems;
        GroupDataTable mExcludedFromGroup;

        bool mFirstRedo;
};

#endif /* UBGRAPHICSITEMUNDOCOMMAND_H_ */
