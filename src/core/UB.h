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
        ChangeFill,
        Drawing
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
