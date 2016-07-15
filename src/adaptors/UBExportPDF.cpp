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



#include "UBExportPDF.h"

#include <QtCore>
#include <QtSvg>

#include "core/UBApplication.h"
#include "core/UBSettings.h"
#include "core/UBSetting.h"
#include "core/UBPersistenceManager.h"

#include "domain/UBGraphicsScene.h"
#include "domain/UBGraphicsSvgItem.h"

#include "document/UBDocumentProxy.h"
#include "document/UBDocumentController.h"

#include "pdf/GraphicsPDFItem.h"

#include "core/memcheck.h"

UBExportPDF::UBExportPDF(QObject *parent)
    : UBExportAdaptor(parent)
{
    // NOOP
}

UBExportPDF::~UBExportPDF()
{
    // NOOP
}

void UBExportPDF::persist(UBDocumentProxy* pDocumentProxy)
{
    if (!pDocumentProxy)
        return;

    QString filename = askForFileName(pDocumentProxy, tr("Export as PDF File"));

    if (filename.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        UBApplication::showMessage(tr("Exporting document..."));

        persistsDocument(pDocumentProxy, filename);

        UBApplication::showMessage(tr("Export successful."));
        QApplication::restoreOverrideCursor();
    }
}


void UBExportPDF::persistsDocument(UBDocumentProxy* pDocumentProxy, const QString& filename)
{
    QPrinter pdfPrinter;

    qDebug() << "exporting document to PDF" << filename;

    pdfPrinter.setOutputFormat(QPrinter::PdfFormat);
    pdfPrinter.setResolution(UBSettings::settings()->pdfResolution->get().toInt());
    pdfPrinter.setOutputFileName(filename);
	pdfPrinter.setFullPage(true);

    //need to calculate screen resolution
	QDesktopWidget* desktop = UBApplication::desktop();
	int dpiCommon = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
	float scaleFactor = 72.0f / dpiCommon;
	
    QPainter pdfPainter;
	bool painterNeedsBegin = true;

    int existingPageCount = pDocumentProxy->pageCount();

    for(int pageIndex = 0 ; pageIndex < existingPageCount; pageIndex++)
    {
        UBGraphicsScene* scene = UBPersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, pageIndex);
        UBApplication::showMessage(tr("Exporting page %1 of %2").arg(pageIndex + 1).arg(existingPageCount));
        // set background to white, no crossing for PDF output
        bool isDark = scene->isDarkBackground();
        bool isCrossed = scene->isCrossedBackground();
        scene->setBackground(false, false);

		QSize pageSize = scene->nominalSize();

        // set high res rendering
        scene->setRenderingQuality(UBItem::RenderingQualityHigh);
        scene->setRenderingContext(UBGraphicsScene::NonScreen);

		//setting page size to appropriate value
		pdfPrinter.setPaperSize(QSizeF(pageSize.width()*scaleFactor, pageSize.height()*scaleFactor), QPrinter::Point);
		if(painterNeedsBegin) painterNeedsBegin = !pdfPainter.begin(&pdfPrinter);
        //render to PDF
		scene->render(&pdfPainter, QRectF(), scene->normalizedSceneRect());

        if (pageIndex < existingPageCount - 1) pdfPrinter.newPage();

        //restore screen rendering quality
        scene->setRenderingContext(UBGraphicsScene::Screen);
        scene->setRenderingQuality(UBItem::RenderingQualityNormal);

        //restore background state
        scene->setBackground(isDark, isCrossed);
    }
	if(!painterNeedsBegin) pdfPainter.end();
}

QString UBExportPDF::exportExtention()
{
    return QString(".pdf");
}

QString UBExportPDF::exportName()
{
    return tr("Export to PDF");
}

bool UBExportPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const UBDocumentTreeModel *docModel = qobject_cast<const UBDocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}
