/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imagecreator.h"

#include "macros.h"

#include <QImageReader>

EXPORT_THUMBNAILER_WITH_JSON(ImageCreator, "imagethumbnail.json")

bool ImageCreator::create(const QString &path, int, int, QImage &img)
{
    // create image preview
    QImageReader ir(path);
    ir.setAutoTransform(true);
    ir.setDecideFormatFromContent(true);
    ir.read(&img);
    if (img.isNull())
        return false;
    if (img.depth() != 32)
        img = img.convertToFormat(img.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    return true;
}

#include "imagecreator.moc"
