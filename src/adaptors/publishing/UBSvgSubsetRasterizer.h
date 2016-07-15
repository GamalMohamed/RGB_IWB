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



#ifndef UBSVGSUBSETRASTERIZER_H_
#define UBSVGSUBSETRASTERIZER_H_

#include <QtGui>

class UBDocumentProxy;

class UBSvgSubsetRasterizer : QObject
{
    Q_OBJECT;

    public:
        UBSvgSubsetRasterizer(UBDocumentProxy* document, int pageIndex, QObject* parent = 0);
        virtual ~UBSvgSubsetRasterizer();

        bool rasterizeToFile(const QString& filename);

    private:
        UBDocumentProxy* mDocument;
        int mPageIndex;

};

#endif /* UBSVGSUBSETRASTERIZER_H_ */
