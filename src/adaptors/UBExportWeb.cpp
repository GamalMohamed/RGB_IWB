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



#include "UBExportWeb.h"

#include "frameworks/UBPlatformUtils.h"
#include "frameworks/UBFileSystemUtils.h"

#include "core/UBDocumentManager.h"
#include "core/UBApplication.h"

#include "document/UBDocumentProxy.h"
#include "document/UBDocumentController.h"

#include "globals/UBGlobals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

#include "core/memcheck.h"

UBExportWeb::UBExportWeb(QObject *parent)
    : UBExportAdaptor(parent)
{
    UBExportWeb::tr("Page"); // dummy slot for translation
}


UBExportWeb::~UBExportWeb()
{
    // NOOP
}


void UBExportWeb::persist(UBDocumentProxy* pDocumentProxy)
{
    if (!pDocumentProxy)
        return;

    QString dirName = askForDirName(pDocumentProxy, tr("Export as Web data"));

    if (dirName.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        UBApplication::showMessage(tr("Exporting document..."));

        if(UBFileSystemUtils::copyDir(pDocumentProxy->persistencePath(), dirName))
        {
            QString htmlPath = dirName + "/index.html";

            QFile html(":www/uniboard-web-player.html");
            html.copy(htmlPath);

            UBApplication::showMessage(tr("Export successful."));

            QDesktopServices::openUrl(QUrl::fromLocalFile(htmlPath));
        }
        else
        {
            UBApplication::showMessage(tr("Export failed."));
        }


        QApplication::restoreOverrideCursor();
    }
}


QString UBExportWeb::exportName()
{
    return tr("Export to Web Browser");
}

bool UBExportWeb::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const UBDocumentTreeModel *docModel = qobject_cast<const UBDocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}
