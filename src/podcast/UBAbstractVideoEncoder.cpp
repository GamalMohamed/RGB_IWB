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



#include "UBAbstractVideoEncoder.h"

#include "core/memcheck.h"

UBAbstractVideoEncoder::UBAbstractVideoEncoder(QObject *pParent)
    : QObject(pParent)
    , mFramesPerSecond(10)
    , mVideoSize(640, 480)
    , mVideoBitsPerSecond(1700000) // 1.7 Mbps
{
    // NOOP

}

UBAbstractVideoEncoder::~UBAbstractVideoEncoder()
{
    // NOOP
}


void UBAbstractVideoEncoder::newChapter(const QString& pLabel, long timestamp)
{
    Q_UNUSED(pLabel);
    Q_UNUSED(timestamp);
}
