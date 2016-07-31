#ifndef UBSETTINGS_H_
#define UBSETTINGS_H_

#include <QtCore>
#include <QtGui>
#include <QtNetwork>

#include "UB.h"
#include "UBSetting.h"

class UBSettings : public QObject
{

    Q_OBJECT

    public:

        static UBSettings* settings();
        static void destroy();
        void init();

    private:

        UBSettings(QObject *parent = 0);
        virtual ~UBSettings();
        void cleanNonPersistentSettings();

    public:

        QStringList* supportedKeyboardSizes;
        void InitKeyboardPaletteKeyBtnSizes();
        void ValidateKeyboardPaletteKeyBtnSize();
        void closing();

        int penWidthIndex();

        qreal currentPenWidth();

        int penColorIndex();
        QColor currentPenColor();
        QColor penColor(bool onDarkBackground);
        QList<QColor> penColors(bool onDarkBackground);

        // Marker related
        int markerWidthIndex();
        qreal currentMarkerWidth();
        int markerColorIndex();
        QColor currentMarkerColor();
        QColor markerColor(bool onDarkBackground);
        QList<QColor> markerColors(bool onDarkBackground);

        // Eraser related
        int eraserWidthIndex();
        qreal eraserFineWidth();
        qreal eraserMediumWidth();
        qreal eraserStrongWidth();
        qreal currentEraserWidth();

        // Background related
        bool isDarkBackground();
        bool isCrossedBackground();
        void setDarkBackground(bool isDarkBackground);
        void setCrossedBackground(bool isCrossedBackground);

        // Stylus palette related
        bool isStylusPaletteVisible();

        // Text related
        QString fontFamily();
        void setFontFamily(const QString &family);
        int fontPixelSize();
        void setFontPixelSize(int pixelSize);
        int fontPointSize();
        void setFontPointSize(int pointSize);
        bool isBoldFont();
        void setBoldFont(bool bold);
        bool isItalicFont();
        void setItalicFont(bool italic);
        bool isUnderlineFont();
        void setUnderlineFont(bool underline);

        void setPassword(const QString& id, const QString& password);
        QString password(const QString& id);
        void removePassword(const QString& id);

        QString proxyUsername();
        void setProxyUsername(const QString& username);
        QString proxyPassword();
        void setProxyPassword(const QString& password);

        QString communityUsername();
        void setCommunityUsername(const QString& username);
        QString communityPassword();
        void setCommunityPassword(const QString& password);
        bool getCommunityDataPersistence(){return communityCredentialsPersistence->get().toBool();}
        void setCommunityPersistence(const bool persistence);

        int libraryIconSize();
        void setLibraryIconsize(const int& size);

        //user directories
        static QString userDataDirectory();
        static QString userDocumentDirectory();
        static QString userBookmarkDirectory();
        static QString userFavoriteListFilePath();
        static QString userTrashDirPath();
        static QString userImageDirectory();
        static QString userVideoDirectory();
        static QString userAudioDirectory();
        static QString userSearchDirectory();
        static QString userAnimationDirectory();
        static QString userInteractiveDirectory();
        static QString userApplicationDirectory();
        static QString userWidgetPath();
        static QString userRelativeWidgetPath();
        static QString userInteractiveFavoritesDirectory();
        static QString userPodcastRecordingDirectory();

        QString userGipLibraryDirectory();

        //application directory
        QString applicationShapeLibraryDirectory();
        QString applicationImageLibraryDirectory();
        QString applicationApplicationsLibraryDirectory();
        QString applicationInteractivesDirectory();
        QString applicationCustomizationDirectory();
        QString applicationCustomFontDirectory();
        QString applicationAudiosLibraryDirectory();
        QString applicationVideosLibraryDirectory();
        QString applicationAnimationsLibraryDirectory();
        QString applicationStartupHintsDirectory();

        QNetworkProxy* httpProxy();

        static int pointerDiameter;
        static int boardMargin;

        static QColor crossDarkBackground;
        static QColor crossLightBackground;
        static QColor paletteColor;
        static QColor opaquePaletteColor;

        static QColor documentViewLightColor;

        static QBrush eraserBrushDarkBackground;
        static QBrush eraserBrushLightBackground;

        static QPen eraserPenDarkBackground;
        static QPen eraserPenLightBackground;

        static QColor documentSizeMarkColorDarkBackground;
        static QColor documentSizeMarkColorLightBackground;

        static int crossSize;
        static int colorPaletteSize;
        static int objectFrameWidth;

        static QString documentGroupName;
        static QString documentName;
        static QString documentSize;
        static QString documentIdentifer;
        static QString documentVersion;
        static QString documentUpdatedAt;

        static QString sessionTitle;
        static QString sessionAuthors;
        static QString sessionObjectives;
        static QString sessionKeywords;
        static QString sessionGradeLevel;
        static QString sessionSubjects;
        static QString sessionType;
        static QString sessionLicence;

        // Issue 1684 - ALTI/AOU - 20131210
        static QString documentDefaultBackgroundImage;
        static QString documentDefaultBackgroundImageDisposition;
        // Fin Issue 1684 - ALTI/AOU - 20131210

        //Issue N/C - NNE - 20140526
        static QString documentTagVersion;
        //Issue N/C - NNE - 20140526

        static QString documentDate;

        static QString trashedDocumentGroupNamePrefix;

        static QString currentFileVersion;

        static QString uniboardDocumentNamespaceUri;
        static QString uniboardApplicationNamespaceUri;

        static const int maxThumbnailWidth;
        static const int defaultThumbnailWidth;
        static const int defaultLibraryIconSize;

        static const int defaultImageWidth;
        static const int defaultShapeWidth;
        static const int defaultWidgetIconWidth;
        static const int defaultVideoWidth;
        static const int defaultGipWidth;
        static const int defaultSoundWidth;

        static const int thumbnailSpacing;
        static const int longClickInterval;

        static const qreal minScreenRatio;

        static QStringList bitmapFileExtensions;
        static QStringList vectoFileExtensions;
        static QStringList imageFileExtensions;
        static QStringList widgetFileExtensions;
        static QStringList interactiveContentFileExtensions;

        static QColor treeViewBackgroundColor;

        static int objectInControlViewMargin;

        static QString appPingMessage;

        UBSetting* productWebUrl;

        QString softwareHomeUrl;

        UBSetting* appToolBarPositionedAtTop;
        UBSetting* appToolBarDisplayText;
        UBSetting* appEnableAutomaticSoftwareUpdates;
        UBSetting* appEnableSoftwareUpdates;
        UBSetting* appToolBarOrientationVertical;
        UBSetting* appPreferredLanguage;

        UBSetting* appDrawingPaletteOrientationHorizontal;

        UBSetting* appIsInSoftwareUpdateProcess;

        UBSetting* appLastSessionDocumentUUID;
        UBSetting* appLastSessionPageIndex;

        UBSetting* appUseMultiscreen;

        UBSetting* appStartupHintsEnabled;

        UBSetting* boardPenFineWidth;
        UBSetting* boardPenMediumWidth;
        UBSetting* boardPenStrongWidth;

        UBSetting* boardMarkerFineWidth;
        UBSetting* boardMarkerMediumWidth;
        UBSetting* boardMarkerStrongWidth;

        UBSetting* boardPenPressureSensitive;
        UBSetting* boardMarkerPressureSensitive;

        UBSetting* boardUseHighResTabletEvent;

        UBSetting* boardKeyboardPaletteKeyBtnSize;

        UBSetting* appStartMode;

        UBSetting* featureSliderPosition;

        UBColorListSetting* boardPenLightBackgroundColors;
        UBColorListSetting* boardPenLightBackgroundSelectedColors;

        UBColorListSetting* boardPenDarkBackgroundColors;
        UBColorListSetting* boardPenDarkBackgroundSelectedColors;

        UBSetting* userCrossDarkBackground;
        UBSetting* userCrossLightBackground;

        UBSetting* boardMarkerAlpha;

        UBColorListSetting* boardMarkerLightBackgroundColors;
        UBColorListSetting* boardMarkerLightBackgroundSelectedColors;

        UBColorListSetting* boardMarkerDarkBackgroundColors;
        UBColorListSetting* boardMarkerDarkBackgroundSelectedColors;

        UBSetting* webUseExternalBrowser;
        UBSetting* webShowPageImmediatelyOnMirroredScreen;

        UBSetting* webHomePage;
        UBSetting* webBookmarksPage;
        UBSetting* webAddBookmarkUrl;
        UBSetting* webShowAddBookmarkButton;

        UBSetting* pageCacheSize;

        UBSetting* boardZoomFactor;

        UBSetting* mirroringRefreshRateInFps;

        UBSetting* lastImportFilePath;
        UBSetting* lastImportFolderPath;

        UBSetting* lastExportFilePath;
        UBSetting* lastExportDirPath;

        UBSetting* lastImportToLibraryPath;

        UBSetting* lastPicturePath;
        UBSetting* lastWidgetPath;
        UBSetting* lastVideoPath;

        UBSetting* appOnlineUserName;

        UBSetting* boardShowToolsPalette;

        QMap<DocumentSizeRatio::Enum, QSize> documentSizes;

        UBSetting* svgViewBoxMargin;
        UBSetting* pdfMargin;
        UBSetting* pdfPageFormat;
        UBSetting* pdfResolution;

        UBSetting* podcastFramesPerSecond;
        UBSetting* podcastVideoSize;
        UBSetting* podcastWindowsMediaBitsPerSecond;
        UBSetting* podcastAudioRecordingDevice;
        UBSetting* podcastQuickTimeQuality;

        UBSetting* podcastPublishToYoutube;
        UBSetting* youTubeUserEMail;
        UBSetting* youTubeCredentialsPersistence;

        UBSetting* uniboardWebEMail;
        UBSetting* uniboardWebAuthor;
        UBSetting* uniboardWebGoogleMapApiKey;

        UBSetting* podcastPublishToIntranet;
        UBSetting* intranetPodcastPublishingUrl;
        UBSetting* intranetPodcastAuthor;

        UBSetting* favoritesNativeToolUris;

        UBSetting* replyWWSerialPort;
        UBSetting* replyPlusConnectionURL;
        UBSetting* replyPlusAddressingMode;

        UBSetting* replyPlusMaxKeypads;

        UBSetting* documentThumbnailWidth;
        UBSetting* imageThumbnailWidth;
        UBSetting* videoThumbnailWidth;
        UBSetting* shapeThumbnailWidth;
        UBSetting* gipThumbnailWidth;
        UBSetting* soundThumbnailWidth;

        UBSetting* rightLibPaletteBoardModeWidth;
        UBSetting* rightLibPaletteBoardModeIsCollapsed;
        UBSetting* rightLibPaletteDesktopModeWidth;
        UBSetting* rightLibPaletteDesktopModeIsCollapsed;
        UBSetting* rightLibPaletteWebModeWidth;
        UBSetting* rightLibPaletteWebModeIsCollapsed;
        UBSetting* leftLibPaletteBoardModeWidth;
        UBSetting* leftLibPaletteBoardModeIsCollapsed;
        UBSetting* leftLibPaletteDesktopModeWidth;
        UBSetting* leftLibPaletteDesktopModeIsCollapsed;

        UBSetting* communityUser;
        UBSetting* communityPsw;
        UBSetting* communityCredentialsPersistence;

        UBSetting* pageSize;
        UBSetting* pageDpi;

        UBSetting* KeyboardLocale;
        UBSetting* swapControlAndDisplayScreens;

        UBSetting* angleTolerance;
        UBSetting* historyLimit;
        UBSetting* teacherGuidePageZeroActivated;
        UBSetting* teacherGuideLessonPagesActivated;

        UBSetting* libIconSize;

        UBSetting* magnifierDrawingMode;

        UBSetting *cacheKeepAspectRatio;
        UBSetting *cacheMode;
        UBSetting *casheLastHoleSize;
        UBSetting *cacheColor;

    public slots:

        void setPenWidthIndex(int index);
        void setPenColorIndex(int index);

        void setMarkerWidthIndex(int index);
        void setMarkerColorIndex(int index);

        void setEraserWidthIndex(int index);
        void setEraserFineWidth(qreal width);
        void setEraserMediumWidth(qreal width);
        void setEraserStrongWidth(qreal width);

         void setStylusPaletteVisible(bool visible);

        void setPenPressureSensitive(bool sensitive);
        void setMarkerPressureSensitive(bool sensitive);

        QVariant value ( const QString & key, const QVariant & defaultValue = QVariant() ) const;
        void setValue (const QString & key,const QVariant & value);

        void colorChanged() { emit colorContextChanged(); }

    signals:
        void colorContextChanged();

    private:

        QSettings* mAppSettings;
        QSettings* mUserSettings;

        static const int sDefaultFontPixelSize;
        static const char *sDefaultFontFamily;

        static QSettings* getAppSettings();

        static QPointer<QSettings> sAppSettings;
        static QPointer<UBSettings> sSingleton;

        static bool checkDirectory(QString& dirPath);
        static QString replaceWildcard(QString& path);

};


#endif /* UBSETTINGS_H_ */
