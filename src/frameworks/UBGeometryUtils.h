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



#ifndef UBGEOMETRYUTILS_H_
#define UBGEOMETRYUTILS_H_

#include <QtGui>

class UBGeometryUtils
{
    private:
        UBGeometryUtils();
        virtual ~UBGeometryUtils();

    public:
        static QPolygonF lineToPolygon(const QLineF& pLine, const qreal& pWidth);
        static QRectF lineToInnerRect(const QLineF& pLine, const qreal& pWidth);

        static QPolygonF arcToPolygon(const QLineF& startRadius, qreal spanAngle, qreal width);

        static QPolygonF lineToPolygon(const QPointF& pStart, const QPointF& pEnd,
                const qreal& pStartWidth, const qreal& pEndWidth);

        static QPointF pointConstrainedInRect(QPointF point, QRectF rect);
        static QPoint pointConstrainedInRect(QPoint point, QRect rect);

        static void crashPointList(QVector<QPointF> &points);

        const static int centimeterGraduationHeight;
        const static int halfCentimeterGraduationHeight;
        const static int millimeterGraduationHeight;
        const static int millimetersPerCentimeter;
        const static int millimetersPerHalfCentimeter;
        const static float inchSize;
};

#endif /* UBGEOMETRYUTILS_H_ */
