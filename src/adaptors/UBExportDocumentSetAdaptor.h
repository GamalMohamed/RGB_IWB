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


#ifndef UBEXPORTDOCUMENTSETADAPTOR_H
#define UBEXPORTDOCUMENTSETADAPTOR_H

#include <QtCore>
#include "UBExportAdaptor.h"
#include "frameworks/UBFileSystemUtils.h"
#include "globals/UBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

class UBDocumentProxy;
class UBDocumentTreeModel;


class UBExportDocumentSetAdaptor : public UBExportAdaptor
{
    Q_OBJECT

    public:
        UBExportDocumentSetAdaptor(QObject *parent = 0);
        virtual ~UBExportDocumentSetAdaptor();

        virtual QString exportName();
        virtual QString exportExtention();

        virtual void persist(UBDocumentProxy* pDocument);
        bool persistData(const QModelIndex &pRootIndex, QString filename);
        bool addDocumentToZip(const QModelIndex &pIndex, UBDocumentTreeModel *model, QuaZip &zip);

        virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);


};

#endif // UBEXPORTDOCUMENTSETADAPTOR_H
