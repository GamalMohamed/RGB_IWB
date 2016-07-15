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



#ifndef UB_H_
#define UB_H_

#include <QtGui>

#define UB_MAX_ZOOM 9

struct UBMimeType
{
    enum Enum
    {
        RasterImage = 0,
        VectorImage,
        AppleWidget,
        W3CWidget,
        Video,
        Audio,
        Flash,
        PDF,
        UniboardTool,
        Group,
        Bookmark,
        Link,
        Web,
        UNKNOWN
    };
};

struct UBStylusTool
{
    enum Enum
    {
        Pen = 0,
        Eraser,
        Marker,
        Selector,
        Play,
        Hand,
        ZoomIn,
        ZoomOut,
        Pointer,
        Line,
        Text,
        Capture,
        RichText,
        ChangeFill,//EV-7 - CFA - 20140122 : pot de peinture
        Drawing //EV-7 - NNE - 20140103
    };
};

struct UBWidth
{
    enum Enum
    {
        Fine = 0, Medium, Strong
    };
};

struct UBZoom
{
    enum Enum
    {
        Small = 0, Medium, Large
    };
};

struct UBSize
{
    enum Enum
    {
        Small = 0, Medium, Large
    };
};

// Deprecated. Keep it for backward campability with old versions
struct UBItemLayerType
{
    enum Enum
    {
        FixedBackground = -2000, Object = -1000, Graphic = 0, Tool = 1000, Control = 2000
    };
};

struct itemLayerType
{
    enum Enum {
        NoLayer = 0
        , BackgroundItem
        , ObjectItem
        , DrawingItem
        , ToolItem
        , CppTool
        , Eraiser
        , Curtain
        , Pointer
        , Cache
        , SelectedItem
    };
};


struct UBGraphicsItemData
{
    enum Enum
    {
        ItemLayerType //Deprecated. Keep it for backward campability with old versions. Use itemLayerType instead
        , ItemLocked
        , ItemEditable//for text only
        , ItemOwnZValue
        , itemLayerType //use instead of deprecated ItemLayerType
        , ItemUuid //storing uuid in QGraphicsItem for fast finding operations
        //Duplicating delegate's functions to make possible working with pure QGraphicsItem
        , ItemFlippable // (bool)
        , ItemRotatable // (bool)
    };
};



struct UBGraphicsItemType
{
    enum Enum
    {
        PolygonItemType = QGraphicsItem::UserType + 1,
        PixmapItemType,
        SvgItemType,
        DelegateButtonType,
        MediaItemType,
        PDFItemType,
        TextItemType,
        CurtainItemType,
        RulerItemType,
        CompassItemType,
        ProtractorItemType,
        StrokeItemType,
        TriangleItemType,
        MagnifierItemType,
        cacheItemType,
        AristoItemType,
        groupContainerType,
        ToolWidgetItemType,
        GraphicsWidgetItemType,
        GraphicsShapeItemType,
        GraphicsPathItemType,
        GraphicsRegularPathItemType,
        GraphicsFreehandItemType,
        GraphicsHandle,
        GraphicsProxyWidget,
        UserTypesCount // this line must be the last line in this enum because it is types counter.
    };
};

struct DocumentSizeRatio
{
    enum Enum
    {
        Ratio4_3 = 0, Ratio16_9, Ratio16_10, Custom
    };
};

#endif /* UB_H_ */
