///////////////////////////////////////////////////////////////////////
// -------------------
// DeKorator window decoration for KDE
// -------------------
// Copyright (C) 2005  moty rahamim <moty.rahamim@gmail.com>
//
// Based on Example-0.8, some ideas and code have been taken from plastik and KCMicons
//
// Example window decoration for KDE
// Copyright (c) 2003, 2004 David Johnson <david@usermode.org>
//
// Plastik KWindowSystem window decoration
// Copyright (C) 2003-2005 Sandro Giessl <sandro@giessl.com>
//
// KCMicons for KDE
// Copyright (c) 2000 Antonio Larrosa <larrosa@kde.org>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.
///////////////////////////////////////////////////////////////////////


#include "deKoratorclient.h"

#include <KDecoration3/DecoratedWindow>
#include <KDecoration3/DecorationSettings>

#include <KConfig>
#include <KConfigGroup>
#include <KIconEffect>
#include <KPluginFactory>

#include <QDir>
#include <QFile>
#include <QPainter>
#include <QStandardPaths>
#include <QTimer>

#include "shadow.h"

K_PLUGIN_FACTORY_WITH_JSON( DeKoratorDecoFactory, "deKorator.json", registerPlugin<DeKorator::Decoration>(); )

using namespace DeKorator;
using KDecoration3::DecorationButtonType;


// global constants

static const int MARGIN = 4;

static const char * const buttonGlyphName[] =
{
    "Restore", "Help", "Max", "Min", "Close",
    "Sticky", "StickyDown", "Above", "AboveDown",
    "Below", "BelowDown", "Shade", "ShadeDown",
    "Menu", "TabClose"
};

static const int ANIMATIONSTEPS = 100;


//////////////////////////////////////////////////////////////////////////////
// Settings                                                                 //
//////////////////////////////////////////////////////////////////////////////

void Settings::read()
{
    // create a config object
    KConfig conf( QStringLiteral( "kwindeKoratorrc" ) );
    const QColor col = QColor( 150, 150, 150 );

    // misc settings
    KConfigGroup config = conf.group( QStringLiteral( "MISC" ) );

    const QString value = config.readEntry( "TitleAlignment", "AlignHCenter" );
    if ( value == QLatin1String( "AlignLeft" ) )
        titleAlign = Qt::AlignLeft;
    else if ( value == QLatin1String( "AlignRight" ) )
        titleAlign = Qt::AlignRight;
    else
        titleAlign = Qt::AlignHCenter;

    useMenuImage = config.readEntry( "UseMenuImage", false );
    ignoreAppIconCol = config.readEntry( "IgnoreAppIconCol", false );
    showBtmBorder = config.readEntry( "ShowBtmBorder", false );
    showMaximizedBorders = config.readEntry( "ShowMaximizedBorders", false );
    useShdText = config.readEntry( "UseShdtext", false );
    activeShdTextOffset = QPoint( config.readEntry( "ActiveShdtextX", 0 ), config.readEntry( "ActiveShdtextY", 0 ) );
    activeShadowColor = config.readEntry( "ActiveShadowColor", col );
    inActiveShdTextOffset = QPoint( config.readEntry( "InActiveShdtextX", 0 ), config.readEntry( "InActiveShdtextY", 0 ) );
    inActiveShadowColor = config.readEntry( "InActiveShadowColor", col );
    btnShift = QPoint( config.readEntry( "BtnShiftX", 0 ), config.readEntry( "BtnShiftY", 0 ) );

    // color's settings
    config = conf.group( QStringLiteral( "COLORS" ) );

    colorizeActFrames = config.readEntry( "ColorizeActFrames", false );
    colorizeActButtons = config.readEntry( "ColorizeActButtons", false );
    colorizeInActFrames = config.readEntry( "ColorizeInActFrames", false );
    colorizeInActButtons = config.readEntry( "ColorizeInActButtons", false );
    useAnimation = config.readEntry( "UseAnimation", false );
    animationType = config.readEntry( "AnimationType", "Intensity" );
    animSteps = config.readEntry( "AnimSteps", 5 );
    animInterval = config.readEntry( "AnimInterval", 30 );
    keepAnimating = config.readEntry( "KeepAnimating", false );
    buttonHoverType = config.readEntry( "ButtonsHoverType", "To Gray" );
    effectAmount = config.readEntry( "EffectAmount", 5 ) / 10.0f;
    activeHighlightColor = config.readEntry( "ActiveHighlightColor", col );
    inActiveHighlightColor = config.readEntry( "InActiveHighlightColor", col );
    decoColorize = config.readEntry( "DecoColorize", "Liquid Method" );
    buttonsColorize = config.readEntry( "ButtonsColorize", "Liquid Method" );

    // buttons settings
    config = conf.group( QStringLiteral( "BUTTONS" ) );

    useCustomButtonsColors = config.readEntry( "UseCustomButtonsColors", false );
    customColorsActiveButtons = config.readEntry( "customColorsActiveButtons", false );
    customColorsInActiveButtons = config.readEntry( "customColorsInActiveButtons", false );
    for ( int i = 0 ; i < buttonTypeAllCount ; ++i )
    {
        cusBtnCol[ i ] = config.readEntry( QLatin1String( buttonGlyphName[ i ] ) + QLatin1String( "ButtonColor" ), col );
    }

    // path's settings
    config = conf.group( QStringLiteral( "PATHS" ) );

    framesPath = config.readEntry( "FramesPath", QString() );
    buttonsPath = config.readEntry( "ButtonsPath", QString() );
    useMasks = config.readEntry( "UseMasks", true );
    masksPath = config.readEntry( "MasksPath", QString() );
}


//////////////////////////////////////////////////////////////////////////////
// Theme                                                                    //
//////////////////////////////////////////////////////////////////////////////

static std::weak_ptr<Theme> s_theme;

std::shared_ptr<Theme> Theme::self()
{
    std::shared_ptr<Theme> theme = s_theme.lock();
    if ( !theme )
    {
        theme = std::make_shared<Theme>();
        theme->reconfigure();
        s_theme = theme;
    }
    return theme;
}

static QString themeSubDir( const QString &localPath, const QString &dirName )
{
    QDir dir( localPath );
    if ( dir.exists( dirName ) )
        return localPath + QLatin1Char( '/' ) + dirName;
    const QString lowerName = dirName.toLower();
    if ( dir.exists( lowerName ) )
        return localPath + QLatin1Char( '/' ) + lowerName;
    return QString();
}

static bool isFramesDir( const QString &path )
{
    return !path.isEmpty() && QFile::exists( path + QLatin1String( "/midTitleBg.png" ) );
}

// Use the first installed theme (preferring the default one) when the
// configured one does not exist, e.g. on first start or with a KDE 4 config.
static void resolveThemePaths( Settings &s )
{
    if ( isFramesDir( s.framesPath ) )
        return;

    QString found;
    const QStringList dirs = QStandardPaths::locateAll( QStandardPaths::GenericDataLocation,
            QStringLiteral( "deKorator/themes" ), QStandardPaths::LocateDirectory );
    for ( const QString &dir : dirs )
    {
        const QStringList entries = QDir( dir ).entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );
        for ( const QString &entry : entries )
        {
            const QString path = dir + QLatin1Char( '/' ) + entry;
            if ( isFramesDir( themeSubDir( path, QStringLiteral( "Deco" ) ) ) &&
                    ( found.isEmpty() || entry == QLatin1String( "Default-theme" ) ) )
                found = path;
        }
    }
    if ( found.isEmpty() )
        return;

    s.framesPath = themeSubDir( found, QStringLiteral( "Deco" ) );
    s.buttonsPath = themeSubDir( found, QStringLiteral( "Buttons" ) );
    s.masksPath = themeSubDir( found, QStringLiteral( "Masks" ) );
}

//////////////////////////////////////////////////////////////////////////////
// reconfigure()
// -------
// Returns true if decorations need to be relayouted/repainted
bool Theme::reconfigure()
{
    Settings s;
    s.read();
    resolveThemePaths( s );

    if ( loaded_ && s == settings_ )
        return false;

    const bool needReload = !loaded_ ||
            s.framesPath != settings_.framesPath ||
            s.buttonsPath != settings_.buttonsPath ||
            s.masksPath != settings_.masksPath;
    settings_ = s;

    if ( needReload )
        loadPixmaps();
    loaded_ = true;

    for ( int wa = 0; wa < WindowActivationStateCount; ++wa )
    {
        decoCache_[ wa ].clear();
        buttonCache_[ wa ].clear();
    }
    return true;
}

// pixels which are dark and opaque in a mask bitmap are cut out of the window
static QRegion regionFromMask( const QImage &mask )
{
    QRegion region;
    const QImage img = mask.convertToFormat( QImage::Format_ARGB32 );
    for ( int y = 0; y < img.height(); ++y )
    {
        const QRgb *line = reinterpret_cast<const QRgb *>( img.constScanLine( y ) );
        int start = -1;
        for ( int x = 0; x <= img.width(); ++x )
        {
            const bool set = x < img.width() && qAlpha( line[ x ] ) >= 128 && qGray( line[ x ] ) < 128;
            if ( set && start < 0 )
                start = x;
            else if ( !set && start >= 0 )
            {
                region += QRect( start, y, x - start, 1 );
                start = -1;
            }
        }
    }
    return region;
}

//////////////////////////////////////////////////////////////////////////////
// loadPixmaps()
// ------------
// Read in the theme images
void Theme::loadPixmaps()
{
    const QString decoPixDir = settings_.framesPath;
    const QString btnPixDir = settings_.buttonsPath;
    const QString masksPixDir = settings_.masksPath;

    // deco
    static const char * const decoPixName[] =
    {
        "topLeftCorner", "leftButtons", "leftTitle", "midTitle", "rightTitle", "rightButtons", "topRightCorner",
        "topLeftFrame", "midLeftFrame", "bottomLeftFrame",
        "leftBottomFrame", "midBottomFrame", "rightBottomFrame",
        "topRightFrame", "midRightFrame", "bottomRightFrame"
    };

    for ( int i = 0; i < decoCount; ++i )
    {
        decoImage_[ i ][ WindowActive ] = QImage( decoPixDir + QLatin1Char( '/' ) + QLatin1String( decoPixName[ i ] ) + QLatin1String( "Bg.png" ) );
    }

    // 0.1 compatibility
    if ( decoImage_[ midLeftFrame ][ WindowActive ].isNull() )
        decoImage_[ midLeftFrame ][ WindowActive ].load( decoPixDir + QLatin1String( "/leftFrameBg.png" ) );
    if ( decoImage_[ midRightFrame ][ WindowActive ].isNull() )
        decoImage_[ midRightFrame ][ WindowActive ].load( decoPixDir + QLatin1String( "/rightFrameBg.png" ) );
    if ( decoImage_[ midBottomFrame ][ WindowActive ].isNull() )
    {
        decoImage_[ leftBottomFrame ][ WindowActive ].load( decoPixDir + QLatin1String( "/buttomLeftCornerBg.png" ) );
        decoImage_[ midBottomFrame ][ WindowActive ].load( decoPixDir + QLatin1String( "/buttomFrameBg.png" ) );
        decoImage_[ rightBottomFrame ][ WindowActive ].load( decoPixDir + QLatin1String( "/buttomRightCornerBg.png" ) );
    }

    for ( int i = 0; i < decoCount; ++i )
    {
        decoImage_[ i ][ WindowInactive ] = QImage( decoPixDir + QLatin1String( "/inactive/" ) + QLatin1String( decoPixName[ i ] ) + QLatin1String( "Bg.png" ) );
        if ( decoImage_[ i ][ WindowInactive ].isNull() )
        {
            decoImage_[ i ][ WindowInactive ] = decoImage_[ i ][ WindowActive ];
        }
    }

    // buttons
    static const char * const buttonStateName[] =
    {
        "", "Hover", "Press"
    };

    static const char * const buttonStatePath[] =
    {
        "/normal/button", "/hover/button", "/press/button"
    };

    for ( int i = 0; i < buttonTypeAllCount; ++i )
    {
        const QString glyph = QLatin1String( buttonGlyphName[ i ] );
        QImage buttonsImage( btnPixDir + QLatin1String( "/buttons" ) + glyph + QLatin1String( ".png" ) );

        if ( !buttonsImage.isNull() && ( buttonsImage.width() % 6 ) == 0 )
        {
            // we got 6 images (active normal/hover/press, inactive normal/hover/press)
            const int buttonWidth = buttonsImage.width() / 6;
            for ( int j = 0; j < buttonStateCount; ++j )
            {
                buttonImage_[ i ][ j ][ WindowActive ] = buttonsImage.copy( j * buttonWidth, 0, buttonWidth, buttonsImage.height() );
                buttonImage_[ i ][ j ][ WindowInactive ] = buttonsImage.copy( ( j + 3 ) * buttonWidth, 0, buttonWidth, buttonsImage.height() );
            }
        }
        else
        // 0.2 compatibility
        {
            for ( int j = 0; j < buttonStateCount; ++j )
            {
                buttonImage_[ i ][ j ][ WindowActive ] = QImage( btnPixDir + QLatin1String( buttonStatePath[ j ] ) + glyph + QLatin1String( buttonStateName[ j ] ) + QLatin1String( ".png" ) );
                buttonImage_[ i ][ j ][ WindowInactive ] = QImage();
            }
        }

        // 0.1 compatibility
        if ( buttonImage_[ i ][ regular ][ WindowActive ].isNull() )
        {
            buttonImage_[ i ][ regular ][ WindowActive ].load( btnPixDir + QLatin1String( "/button" ) + glyph + QLatin1String( ".png" ) );
            if ( buttonImage_[ i ][ regular ][ WindowActive ].isNull() )
            {
                // no restore button, use the maximize button
                if ( i == restore )
                {
                    const QString maxGlyph = QLatin1String( buttonGlyphName[ max ] );
                    for ( int j = 0; j < buttonStateCount; ++j )
                    {
                        buttonImage_[ i ][ j ][ WindowActive ] = QImage( btnPixDir + QLatin1String( buttonStatePath[ j ] ) + maxGlyph + QLatin1String( buttonStateName[ j ] ) + QLatin1String( ".png" ) );
                    }
                    if ( buttonImage_[ i ][ regular ][ WindowActive ].isNull() )
                    {
                        buttonImage_[ i ][ regular ][ WindowActive ].load( btnPixDir + QLatin1String( "/button" ) + maxGlyph + QLatin1String( ".png" ) );
                    }
                }
            }
            for ( int j = hover; j < buttonStateCount; ++j )
            {
                buttonImage_[ i ][ j ][ WindowActive ] = buttonImage_[ i ][ regular ][ WindowActive ];
            }
        }

        for ( int j = 0; j < buttonStateCount; ++j )
        {
            if ( buttonImage_[ i ][ j ][ WindowInactive ].isNull() )
            {
                buttonImage_[ i ][ j ][ WindowInactive ] = buttonImage_[ i ][ j ][ WindowActive ];
            }
        }
    }

    // masks
    static const char * const maskName[] =
    {
        "topLeftCorner", "topMid", "topRightCorner", "bottomLeftCorner", "bottomMid", "bottomRightCorner"
    };
    // 0.2 compatibility
    static const char * const oldMaskName[] =
    {
        nullptr, nullptr, nullptr, "buttomLeftCorner", "buttomMid", "buttomRightCorner"
    };

    for ( int i = 0; i < maskCount; ++i )
    {
        QImage mask( masksPixDir + QLatin1Char( '/' ) + QLatin1String( maskName[ i ] ) + QLatin1String( "Bitmap.png" ) );
        if ( mask.isNull() && oldMaskName[ i ] )
            mask.load( masksPixDir + QLatin1Char( '/' ) + QLatin1String( oldMaskName[ i ] ) + QLatin1String( "Bitmap.png" ) );
        mask_[ i ] = regionFromMask( mask );
        maskSize_[ i ] = mask.size();
    }

    determineSizes();
}

//////////////////////////////////////////////////////////////////////////////
// determineSizes()
// ------------
//
void Theme::determineSizes()
{
    // frames
    titleSize = decoImage_[ midTitle ][ WindowActive ].height();
    leftFrameSize = decoImage_[ midLeftFrame ][ WindowActive ].width();
    bottomFrameSize = decoImage_[ midBottomFrame ][ WindowActive ].height();
    rightFrameSize = decoImage_[ midRightFrame ][ WindowActive ].width();

    topLeftCornerWidth = decoImage_[ topLeftCorner ][ WindowActive ].width();
    topRightCornerWidth = decoImage_[ topRightCorner ][ WindowActive ].width();
}

QSize Theme::buttonSize( buttonTypeAll type ) const
{
    return buttonImage_[ type ][ regular ][ WindowActive ].size();
}

//////////////////////////////////////////////////////////////////////////////
// decoPixmap()
// ------------
// frame images, colorized if requested
QPixmap Theme::decoPixmap( decoType part, bool active, const QColor &titleBarColor )
{
    const WindowActivationState wa = active ? WindowActive : WindowInactive;
    const bool colorize = active ? settings_.colorizeActFrames : settings_.colorizeInActFrames;
    const QRgb key = colorize ? titleBarColor.rgba() : 0;

    auto it = decoCache_[ wa ].constFind( key );
    if ( it == decoCache_[ wa ].constEnd() )
    {
        QList<QPixmap> pixmaps;
        for ( int i = 0; i < decoCount; ++i )
        {
            const QImage &image = decoImage_[ i ][ wa ];
            pixmaps.append( QPixmap::fromImage( colorize ? colorizedImage( image, titleBarColor, settings_.decoColorize ) : image ) );
        }
        it = decoCache_[ wa ].insert( key, pixmaps );
    }
    return it->at( part );
}

//////////////////////////////////////////////////////////////////////////////
// buttonImage()
// ------------
// button images, colorized if requested
QImage Theme::buttonImage( buttonTypeAll type, buttonState state, bool active, const QColor &buttonColor )
{
    const WindowActivationState wa = active ? WindowActive : WindowInactive;
    const bool colorize = active ? settings_.colorizeActButtons : settings_.colorizeInActButtons;
    const bool customColors = settings_.useCustomButtonsColors &&
            ( active ? settings_.customColorsActiveButtons : settings_.customColorsInActiveButtons );
    const QRgb key = !colorize ? 0 : customColors ? 1 : buttonColor.rgba();

    auto it = buttonCache_[ wa ].constFind( key );
    if ( it == buttonCache_[ wa ].constEnd() )
    {
        QList<QImage> images;
        for ( int i = 0; i < buttonTypeAllCount; ++i )
        {
            for ( int j = 0; j < buttonStateCount; ++j )
            {
                const QImage &image = buttonImage_[ i ][ j ][ wa ];
                images.append( colorize ? colorizedImage( image, customColors ? settings_.cusBtnCol[ i ] : buttonColor, settings_.buttonsColorize ) : image );
            }
        }
        it = buttonCache_[ wa ].insert( key, images );
    }
    return it->at( int( type ) * int( buttonStateCount ) + int( state ) );
}


//////////////////////////////////////////////////////////////////////////////
// colorizedImage()
// ------------
//
QImage Theme::colorizedImage( const QImage &image, const QColor &c, const QString &colorizeMethod )
{
    if ( image.isNull() )
        return image;

    if ( colorizeMethod == QLatin1String( "Liquid Method" ) )
    {
        const QImage img = image.convertToFormat( QImage::Format_ARGB32 );
        QImage dest( img.width(), img.height(), QImage::Format_ARGB32 );
        const int srcR = c.red() + 100;
        const int srcG = c.green() + 100;
        const int srcB = c.blue() + 100;

        for ( int y = 0; y < img.height(); ++y )
        {
            const QRgb *data = reinterpret_cast<const QRgb *>( img.constScanLine( y ) );
            QRgb *destData = reinterpret_cast<QRgb *>( dest.scanLine( y ) );
            for ( int x = 0; x < img.width(); ++x )
            {
                const int delta = 255 - qGray( data[ x ] );
                destData[ x ] = qRgba( qBound( 0, srcR - delta, 255 ),
                                       qBound( 0, srcG - delta, 255 ),
                                       qBound( 0, srcB - delta, 255 ),
                                       qAlpha( data[ x ] ) );
            }
        }
        return dest;
    }
    else if ( colorizeMethod == QLatin1String( "KDE Method" ) )
    {
        QImage img = image;
        KIconEffect::colorize( img, c, 1.0 );
        return img;
    }
    else if ( colorizeMethod == QLatin1String( "Hue Adjustment" ) )
    {
        const QImage img = image.convertToFormat( QImage::Format_ARGB32 );
        QImage dest( img.width(), img.height(), QImage::Format_ARGB32 );
        int h, s, v;
        c.getHsv( &h, &s, &v );
        const int ch = h;
        QColor col;

        for ( int y = 0; y < img.height(); ++y )
        {
            const QRgb *data = reinterpret_cast<const QRgb *>( img.constScanLine( y ) );
            QRgb *destData = reinterpret_cast<QRgb *>( dest.scanLine( y ) );
            for ( int x = 0; x < img.width(); ++x )
            {
                col.setRgb( data[ x ] );
                col.getHsv( &h, &s, &v );
                col.setHsv( ch, s, v );
                destData[ x ] = qRgba( col.red(), col.green(), col.blue(), qAlpha( data[ x ] ) );
            }
        }
        return dest;
    }
    return image;
}

//////////////////////////////////////////////////////////////////////////////
// maskedRegion()
// -------------------
// the visible part of a window of the given size, with the theme's
// corner masks cut out (this was doShape() in KDE 4)
QRegion Theme::maskedRegion( const QSize &size, bool withBottom ) const
{
    const int w = size.width();
    const int h = size.height();
    QRegion mask( 0, 0, w, h );

    const int topLeftW = maskSize_[ topLeftMask ].width();
    const int topMidW = maskSize_[ topMidMask ].width();
    const int topRightW = maskSize_[ topRightMask ].width();

    // top left
    mask -= mask_[ topLeftMask ];

    // top mid
    if ( topMidW >= 1 )
    {
        int pos = topLeftW;
        const int rep = ( w - topLeftW - topRightW ) / topMidW;

        for ( int i = 0 ; i < rep ; ++i )
        {
            mask -= mask_[ topMidMask ].translated( topLeftW + ( i * topMidW ), 0 );
            pos += topMidW;
        }

        QRegion m = mask_[ topMidMask ] - mask_[ topMidMask ].translated( w - pos - topRightW, 0 );
        mask -= m.translated( pos, 0 );
    }

    //top right
    mask -= mask_[ topRightMask ].translated( w - topRightW, 0 );

    if ( withBottom )
    {
        const int bottomLeftW = maskSize_[ bottomLeftMask ].width();
        const int bottomMidW = maskSize_[ bottomMidMask ].width();
        const int bottomRightW = maskSize_[ bottomRightMask ].width();

        //bottom left
        mask -= mask_[ bottomLeftMask ].translated( 0, h - maskSize_[ bottomLeftMask ].height() );

        // bottom mid
        if ( bottomMidW >= 1 )
        {
            int pos = bottomLeftW;
            const int rep = ( w - bottomLeftW - bottomRightW ) / bottomMidW;
            const int hm = h - maskSize_[ bottomMidMask ].height();

            for ( int i = 0 ; i < rep ; ++i )
            {
                mask -= mask_[ bottomMidMask ].translated( bottomLeftW + ( i * bottomMidW ), hm );
                pos += bottomMidW;
            }

            QRegion m = mask_[ bottomMidMask ] - mask_[ bottomMidMask ].translated( w - pos - bottomRightW, 0 );
            mask -= m.translated( pos, hm );
        }

        //bottom right
        mask -= mask_[ bottomRightMask ].translated( w - bottomRightW, h - maskSize_[ bottomRightMask ].height() );
    }

    return mask;
}


//////////////////////////////////////////////////////////////////////////////
// Image effects (formerly from QImageBlitz)                                //
//////////////////////////////////////////////////////////////////////////////

static QImage intensityEffect( const QImage &image, float percent )
{
    QImage img = image.convertToFormat( QImage::Format_ARGB32 );
    for ( int y = 0; y < img.height(); ++y )
    {
        QRgb *data = reinterpret_cast<QRgb *>( img.scanLine( y ) );
        for ( int x = 0; x < img.width(); ++x )
        {
            const QRgb p = data[ x ];
            data[ x ] = qRgba( qMin( 255, qRed( p ) + int( qRed( p ) * percent ) ),
                               qMin( 255, qGreen( p ) + int( qGreen( p ) * percent ) ),
                               qMin( 255, qBlue( p ) + int( qBlue( p ) * percent ) ),
                               qAlpha( p ) );
        }
    }
    return img;
}

// Moves each pixel towards color by val (0.0 - 1.0). Larger values overflow,
// which is what the "Trashed" effect relies on.
static QImage fadeEffect( const QImage &image, float val, const QColor &color )
{
    QImage img = image.convertToFormat( QImage::Format_ARGB32 );
    const int cr = color.red(), cg = color.green(), cb = color.blue();
    auto fade = [val]( int c, int target ) {
        return c > target ? c - int( val * ( c - target ) + 0.5f ) : c + int( val * ( target - c ) + 0.5f );
    };
    for ( int y = 0; y < img.height(); ++y )
    {
        QRgb *data = reinterpret_cast<QRgb *>( img.scanLine( y ) );
        for ( int x = 0; x < img.width(); ++x )
        {
            const QRgb p = data[ x ];
            data[ x ] = qRgba( fade( qRed( p ), cr ), fade( qGreen( p ), cg ), fade( qBlue( p ), cb ), qAlpha( p ) );
        }
    }
    return img;
}

static QImage solarizeEffect( const QImage &image, int factor )
{
    QImage img = image.convertToFormat( QImage::Format_ARGB32 );
    const int threshold = 255 * factor / 100;
    auto solarize = [threshold]( int c ) { return c > threshold ? 255 - c : c; };
    for ( int y = 0; y < img.height(); ++y )
    {
        QRgb *data = reinterpret_cast<QRgb *>( img.scanLine( y ) );
        for ( int x = 0; x < img.width(); ++x )
        {
            const QRgb p = data[ x ];
            data[ x ] = qRgba( solarize( qRed( p ) ), solarize( qGreen( p ) ), solarize( qBlue( p ) ), qAlpha( p ) );
        }
    }
    return img;
}


//////////////////////////////////////////////////////////////////////////////
// Button Class                                                             //
//////////////////////////////////////////////////////////////////////////////

Button::Button( DecorationButtonType type, Decoration *decoration, QObject *parent )
        : KDecoration3::DecorationButton( type, decoration, parent )
{
    animTmr_ = new QTimer( this );
    animTmr_->setSingleShot( true );
    connect( animTmr_, &QTimer::timeout, this, &Button::animate );
    connect( this, &DecorationButton::hoveredChanged, this, &Button::hoverChange );

    KDecoration3::DecoratedWindow *w = decoration->window();
    switch ( type )
    {
    case DecorationButtonType::Maximize:
        // middle & right buttons maximize vertically / horizontally
        setAcceptedButtons( Qt::LeftButton | Qt::MiddleButton | Qt::RightButton );
        setVisible( w->isMaximizeable() );
        connect( w, &KDecoration3::DecoratedWindow::maximizeableChanged, this, &Button::setVisible );
        break;
    case DecorationButtonType::Minimize:
        setVisible( w->isMinimizeable() );
        connect( w, &KDecoration3::DecoratedWindow::minimizeableChanged, this, &Button::setVisible );
        break;
    case DecorationButtonType::Close:
        setVisible( w->isCloseable() );
        connect( w, &KDecoration3::DecoratedWindow::closeableChanged, this, &Button::setVisible );
        break;
    case DecorationButtonType::ContextHelp:
        setVisible( w->providesContextHelp() );
        connect( w, &KDecoration3::DecoratedWindow::providesContextHelpChanged, this, &Button::setVisible );
        break;
    case DecorationButtonType::Shade:
        setVisible( w->isShadeable() );
        connect( w, &KDecoration3::DecoratedWindow::shadeableChanged, this, &Button::setVisible );
        break;
    default:
        break;
    }

    // the checked variant (e.g. restore instead of maximize) may differ in size
    connect( this, &DecorationButton::checkedChanged, this, &Button::updateSize );
    updateSize();
}

KDecoration3::DecorationButton *Button::create( DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent )
{
    Decoration *d = qobject_cast<Decoration *>( decoration );
    if ( !d )
        return nullptr;

    switch ( type )
    {
    case DecorationButtonType::Menu:
    case DecorationButtonType::OnAllDesktops:
    case DecorationButtonType::Minimize:
    case DecorationButtonType::Maximize:
    case DecorationButtonType::Close:
    case DecorationButtonType::ContextHelp:
    case DecorationButtonType::Shade:
    case DecorationButtonType::KeepBelow:
    case DecorationButtonType::KeepAbove:
    case DecorationButtonType::Spacer:
        return new Button( type, d, parent );
    default:
        // no images for the application menu and exclude from capture buttons
        return nullptr;
    }
}

Decoration *Button::deco() const
{
    return static_cast<Decoration *>( decoration() );
}

//////////////////////////////////////////////////////////////////////////////
// glyph()
// ---------------
// The image to use, depending on the button type and its state
buttonTypeAll Button::glyph() const
{
    switch ( type() )
    {
    case DecorationButtonType::Menu:
        return menu;
    case DecorationButtonType::OnAllDesktops:
        return isChecked() ? stickydown : sticky;
    case DecorationButtonType::Minimize:
        return min;
    case DecorationButtonType::Maximize:
        return isChecked() ? restore : max;
    case DecorationButtonType::Close:
        return close;
    case DecorationButtonType::ContextHelp:
        return help;
    case DecorationButtonType::Shade:
        return isChecked() ? shadedown : shade;
    case DecorationButtonType::KeepBelow:
        return isChecked() ? belowdown : below;
    case DecorationButtonType::KeepAbove:
        return isChecked() ? abovedown : above;
    default:
        return help;
    }
}

//////////////////////////////////////////////////////////////////////////////
// updateSize()
// ----------
// Size is given by the button image, height by the title bar
void Button::updateSize()
{
    Theme *theme = deco()->theme();
    int width = 3; // spacer

    if ( type() != DecorationButtonType::Spacer )
    {
        width = theme->buttonSize( glyph() ).width();
        if ( type() == DecorationButtonType::Menu )
            width = qMax( 16, width );
    }
    setGeometry( QRectF( geometry().topLeft(), QSizeF( width, theme->titleSize ) ) );
}

//////////////////////////////////////////////////////////////////////////////
// hoverChange()
// ------------
// Mouse has entered or left the button
void Button::hoverChange( bool hovered )
{
    const Settings &s = deco()->theme()->settings();

    if ( hovered )
        step_ = s.animSteps;

    if ( s.useAnimation )
        animate();
    else
        update();
}

//////////////////////////////////////////////////////////////////////////////
// paint()
// ------------
// Draw the button
void Button::paint( QPainter *painter, const QRectF & /*repaintArea*/ )
{
    if ( !isVisible() || type() == DecorationButtonType::Spacer )
        return;

    Decoration *d = deco();
    Theme *theme = d->theme();
    const Settings &s = theme->settings();
    const bool act = d->window()->isActive();
    const QRectF g = geometry();
    int dx = 0, dy = 0;
    QImage image;
    bool appIcon = false;

    // apply app icon effects
    if ( type() == DecorationButtonType::Menu && !s.useMenuImage )
    {
        appIcon = true;
        dy = ( theme->titleSize - 16 ) / 2;

        image = d->window()->icon().pixmap( QSize( 16, 16 ), painter->device()->devicePixelRatioF() ).toImage();

        if ( !s.ignoreAppIconCol )
        {
            if ( s.useCustomButtonsColors && ( act ? s.customColorsActiveButtons : s.customColorsInActiveButtons ) )
            {
                image = Theme::colorizedImage( image, s.cusBtnCol[ menu ], s.buttonsColorize );
            }
            else if ( act ? s.colorizeActButtons : s.colorizeInActButtons )
            {
                image = Theme::colorizedImage( image, d->buttonColor( act ), s.buttonsColorize );
            }
        }
    }
    else
    {
        image = theme->buttonImage( glyph(), isPressed() ? press : isHovered() ? hover : regular, act, d->buttonColor( act ) );
        dy = ( theme->titleSize - image.height() ) / 2;
    }

    // down
    if ( isPressed() )
    {
        dx += s.btnShift.x();
        dy += s.btnShift.y();

        image = chooseRightHoverEffect( image, s.buttonHoverType );
    }
    // hover
    else if ( isHovered() )
    {
        image = chooseRightHoverEffect( image, s.useAnimation ? s.animationType : s.buttonHoverType );
    }
    // regular
    else
    {
        if ( s.useAnimation && animProgress_ > 0 )
            image = chooseRightHoverEffect( image, s.animationType );
    }

    if ( appIcon )
        painter->drawImage( QRectF( g.x() + dx, g.y() + dy, 16, 16 ), image );
    else
        painter->drawImage( QPointF( g.x() + dx, g.y() + dy ), image );

    if ( d->window()->isShaded() && !s.showBtmBorder )
    {
        painter->setPen( QColor( 70, 70, 70 ) );
        painter->drawLine( QPointF( g.left(), g.top() + theme->titleSize - 1 ), QPointF( g.right() - 1, g.top() + theme->titleSize - 1 ) );
    }
}

//////////////////////////////////////////////////////////////////////////////
// chooseRightHoverEffect()
// ------------
//
QImage Button::chooseRightHoverEffect( const QImage &image, const QString &res ) const
{
    const Settings &s = deco()->theme()->settings();
    const bool act = deco()->window()->isActive();
    QImage img = image;

    if ( s.useAnimation && !isPressed() )
    {
        float f = ( float ) animProgress_ / 100;

        if ( res == QLatin1String( "Intensity" ) )
        {
            img = intensityEffect( img, f / 1.5f );
        }
        else if ( res == QLatin1String( "Blend" ) )
        {
            img = fadeEffect( img, f / 1.7f, act ? s.activeHighlightColor : s.inActiveHighlightColor );
        }
        else if ( res == QLatin1String( "Solarize" ) )
        {
            img = solarizeEffect( img, 100 - animProgress_ );
        }
        else if ( res == QLatin1String( "Fade" ) )
        {
            img = fadeEffect( img, f / 3, act ? s.activeHighlightColor : s.inActiveHighlightColor );
        }
        else if ( res == QLatin1String( "Trashed" ) )
        {
            img = fadeEffect( img, animProgress_, Qt::black );
        }
        else if ( res == QLatin1String( "Hue Shift" ) )
        {
            int h, sat, v;
            QColor col;
            const int shift = ( int ) ( animProgress_ * 3.5 );
            col.setRgb( 255, 0, 0 );
            col.getHsv( &h, &sat, &v );
            col.setHsv( shift, sat, v );

            img = Theme::colorizedImage( img, col, QStringLiteral( "Hue Adjustment" ) );
        }
    }
    else
    {
        if ( s.buttonHoverType == QLatin1String( "To Gray" ) )
            KIconEffect::toGray( img, s.effectAmount );
        else if ( s.buttonHoverType == QLatin1String( "Colorize" ) )
            img = Theme::colorizedImage( img, act ? s.activeHighlightColor : s.inActiveHighlightColor, s.buttonsColorize );
        else if ( s.buttonHoverType == QLatin1String( "DeSaturate" ) )
            KIconEffect::deSaturate( img, s.effectAmount );
        else if ( s.buttonHoverType == QLatin1String( "To Gamma" ) )
            KIconEffect::toGamma( img, s.effectAmount );
    }
    return img;
}

//////////////////////////////////////////////////////////////////////////////
// animate()
// ------------
//
void Button::animate()
{
    const Settings &s = deco()->theme()->settings();
    animTmr_->stop();

    if ( isHovered() )
    {
        if ( s.keepAnimating )
        {
            animProgress_ += step_;

            if ( animProgress_ >= ANIMATIONSTEPS || animProgress_ <= 0 )
            {
                animProgress_ = qBound( 0, animProgress_, ANIMATIONSTEPS );
                step_ = -step_;
            }
            animTmr_->start( s.animInterval );
        }
        else
        {
            animProgress_ = qMin( ANIMATIONSTEPS, animProgress_ + s.animSteps );
            if ( animProgress_ < ANIMATIONSTEPS )
                animTmr_->start( s.animInterval );
        }
    }
    else
    {
        animProgress_ = qMax( 0, animProgress_ - s.animSteps );
        if ( animProgress_ > 0 )
            animTmr_->start( s.animInterval );
    }
    update();
}


//////////////////////////////////////////////////////////////////////////////
// Decoration Class                                                         //
//////////////////////////////////////////////////////////////////////////////

// basic layout:

//  __________________________________________________________________________
// | topLeftCorner | left btns |      title       | right btns | topRightCorner|
// |_______________|___________|__________________|____________|_______________|
// | |                                                                     | |
// | |                         window contents                             | |
// | |                                                                     | |
// |left frame                                                  right frame|
// |_|_____________________________________________________________________|_|
// |                                 bottom frame                            |
// |_________________________________________________________________________|
//

Decoration::Decoration( QObject *parent, const QVariantList &args )
        : KDecoration3::Decoration( parent, args )
{
}

Decoration::~Decoration() = default;

bool Decoration::init()
{
    theme_ = Theme::self();

    KDecoration3::DecoratedWindow *w = window();
    auto repaint = [this] { update(); };

    connect( w, &KDecoration3::DecoratedWindow::activeChanged, this, repaint );
    connect( w, &KDecoration3::DecoratedWindow::iconChanged, this, repaint );
    connect( w, &KDecoration3::DecoratedWindow::paletteChanged, this, repaint );
    connect( w, &KDecoration3::DecoratedWindow::captionChanged, this, [this] {
        captionBufferDirty_ = true;
        update( titleBar() );
    } );
    connect( w, &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateLayout );
    connect( w, &KDecoration3::DecoratedWindow::shadedChanged, this, &Decoration::updateLayout );
    connect( w, &KDecoration3::DecoratedWindow::widthChanged, this, &Decoration::updateButtonsGeometry );

    auto s = settings().get();
    connect( s, &KDecoration3::DecorationSettings::reconfigured, this, &Decoration::reconfigure );
    connect( s, &KDecoration3::DecorationSettings::decorationButtonsLeftChanged, this, &Decoration::recreateButtons );
    connect( s, &KDecoration3::DecorationSettings::decorationButtonsRightChanged, this, &Decoration::recreateButtons );
    connect( s, &KDecoration3::DecorationSettings::fontChanged, this, [this] {
        captionBufferDirty_ = true;
        update();
    } );

    recreateButtons();
    updateLayout();
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// reconfigure()
// -------
// The configuration has changed
void Decoration::reconfigure()
{
    // only the first decoration actually reloads the shared theme
    theme_->reconfigure();

    captionBufferDirty_ = true;
    maskSize_ = QSize();
    recreateButtons();
    updateLayout();
}

//////////////////////////////////////////////////////////////////////////////
// recreateButtons()
// ------------
// Create the title bar buttons
void Decoration::recreateButtons()
{
    delete leftButtons_;
    delete rightButtons_;

    leftButtons_ = new KDecoration3::DecorationButtonGroup( KDecoration3::DecorationButtonGroup::Position::Left, this, &Button::create );
    rightButtons_ = new KDecoration3::DecorationButtonGroup( KDecoration3::DecorationButtonGroup::Position::Right, this, &Button::create );

    const auto buttons = leftButtons_->buttons() + rightButtons_->buttons();
    for ( KDecoration3::DecorationButton *button : buttons )
    {
        connect( button, &KDecoration3::DecorationButton::visibilityChanged, this, &Decoration::updateButtonsGeometry );
        connect( button, &KDecoration3::DecorationButton::geometryChanged, this, &Decoration::updateButtonsGeometry );
    }

    updateButtonsGeometry();
}

bool Decoration::hideBorders() const
{
    return window()->isMaximized() && !theme_->settings().showMaximizedBorders;
}

QColor Decoration::titleBarColor( bool active ) const
{
    return window()->color( active ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive, KDecoration3::ColorRole::TitleBar );
}

QColor Decoration::buttonColor( bool active ) const
{
    return window()->palette().color( active ? QPalette::Active : QPalette::Inactive, QPalette::Button );
}

//////////////////////////////////////////////////////////////////////////////
// updateLayout()
// ----------
// Set the size of the borders
void Decoration::updateLayout()
{
    const Settings &s = theme_->settings();
    int l = theme_->leftFrameSize;
    int r = theme_->rightFrameSize;
    int b = theme_->bottomFrameSize;

    if ( hideBorders() )
    {
        l = 0;
        r = 0;
        b = 0;
    }
    if ( window()->isShaded() && !s.showBtmBorder )
        b = 0;

    setBorders( QMarginsF( l, theme_->titleSize, r, b ) );
    updateButtonsGeometry();
}

//////////////////////////////////////////////////////////////////////////////
// updateButtonsGeometry()
// ----------
// Place the button groups next to the corners
void Decoration::updateButtonsGeometry()
{
    if ( !leftButtons_ || !rightButtons_ )
        return;

    const qreal w = size().width();
    const bool noBorders = hideBorders();

    leftButtons_->setPos( QPointF( noBorders ? 0 : theme_->topLeftCornerWidth, 0 ) );
    rightButtons_->setPos( QPointF( w - ( noBorders ? 0 : theme_->topRightCornerWidth ) - rightButtons_->geometry().width(), 0 ) );

    setTitleBar( QRectF( 0, 0, w, theme_->titleSize ) );
    update();
}

//////////////////////////////////////////////////////////////////////////////
// titleRect()
// ----------
// The space between the button groups
QRect Decoration::titleRect() const
{
    const int left = qRound( leftButtons_->geometry().right() );
    const int right = qRound( rightButtons_->geometry().left() );
    return QRect( left, 0, qMax( 0, right - left ), theme_->titleSize );
}

QRegion Decoration::maskRegion() const
{
    const QSize s = size().toSize();
    const bool withBottom = !window()->isShaded() || theme_->settings().showBtmBorder;

    if ( s != maskSize_ || withBottom != maskWithBottom_ )
    {
        mask_ = theme_->maskedRegion( s, withBottom );
        maskSize_ = s;
        maskWithBottom_ = withBottom;
    }
    return mask_;
}


static void drawHorizontalTiled( QPainter *p, const QRect &rect,
                                 const QPixmap &left, const QPixmap &mid, const QPixmap &right )
{
    if ( rect.isEmpty() ) {
        return;
    }
    int leftSize = left.width();
    int rightSize = right.width();
    int sum = leftSize + mid.width() + rightSize;
    if ( mid.width() > 0 ) {
        if ( rect.width() >= sum ) {
            p->drawTiledPixmap( QRect( rect.left() + leftSize, rect.top(), rect.width() - leftSize - rightSize, rect.height() ), mid );
        } else {
            leftSize = leftSize * rect.width() / sum;
            rightSize = rightSize * rect.width() / sum;
            p->drawPixmap( QRect( rect.left() + leftSize, rect.top(), rect.width() - leftSize - rightSize, rect.height() ), mid );
        }
    } else {
        if ( sum <= 0 ) {
            return;
        }
        leftSize = leftSize * rect.width() / sum;
        rightSize = rect.width() - leftSize;
    }
    if ( leftSize > 0 ) p->drawPixmap( QRect( rect.left(), rect.top(), leftSize, rect.height() ), left );
    if ( rightSize > 0 ) p->drawPixmap( QRect( rect.left() + rect.width() - rightSize, rect.top(), rightSize, rect.height() ), right );
}

static void drawVerticalTiled( QPainter *p, const QRect &rect,
                               const QPixmap &top, const QPixmap &mid, const QPixmap &bottom )
{
    if ( rect.isEmpty() ) {
        return;
    }
    int topSize = top.height();
    int bottomSize = bottom.height();
    int sum = topSize + mid.height() + bottomSize;
    if ( mid.height() > 0 ) {
        if ( rect.height() >= sum ) {
            p->drawTiledPixmap( QRect( rect.left(), rect.top() + topSize, rect.width(), rect.height() - topSize - bottomSize ), mid );
        } else {
            topSize = topSize * rect.height() / sum;
            bottomSize = bottomSize * rect.height() / sum;
            p->drawPixmap( QRect( rect.left(), rect.top() + topSize, rect.width(), rect.height() - topSize - bottomSize ), mid );
        }
    } else {
        if ( sum <= 0 ) {
            return;
        }
        topSize = topSize * rect.height() / sum;
        bottomSize = rect.height() - topSize;
    }
    if ( topSize > 0 ) p->drawPixmap( QRect( rect.left(), rect.top(), rect.width(), topSize ), top );
    if ( bottomSize > 0 ) p->drawPixmap( QRect( rect.left(), rect.top() + rect.height() - bottomSize, rect.width(), bottomSize ), bottom );
}

//////////////////////////////////////////////////////////////////////////////
// paint()
// ------------
// Repaint the window
void Decoration::paint( QPainter *painter, const QRectF &repaintArea )
{
    const Settings &s = theme_->settings();
    const bool active = window()->isActive();
    const KDecoration3::ColorGroup colorGroup = active ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive;
    const int TITLESIZE = theme_->titleSize;
    const int w = qRound( size().width() ), h = qRound( size().height() );
    const bool noBorders = hideBorders();

    painter->save();

    if ( !theme_->isValid() )
    {
        // no theme installed, at least show something
        painter->fillRect( rect(), titleBarColor( active ) );
        painter->restore();
        return;
    }

    if ( s.useMasks && !noBorders )
        painter->setClipRegion( maskRegion(), Qt::IntersectClip );

    const QColor titleColor = titleBarColor( active );
    auto pixmap = [this, active, &titleColor]( decoType part ) {
        return theme_->decoPixmap( part, active, titleColor );
    };

    const QRect titleR = titleRect();
    int tx, ty, tw, th;
    titleR.getRect( &tx, &ty, &tw, &th );
    const int leftCornerWidth = noBorders ? 0 : theme_->topLeftCornerWidth;
    const int rightCornerWidth = noBorders ? 0 : theme_->topRightCornerWidth;

    {
        // topLeftCorner
        if ( leftCornerWidth > 0 )
            painter->drawPixmap( QRect( 0, 0, leftCornerWidth, TITLESIZE ), pixmap( topLeftCorner ) );

        // Space under the left button group
        if ( tx > leftCornerWidth )
            painter->drawTiledPixmap( QRect( leftCornerWidth, 0, tx - leftCornerWidth, TITLESIZE ), pixmap( leftButtons ) );

        drawHorizontalTiled( painter, QRect( tx, 0, tw, TITLESIZE ),
                             pixmap( leftTitle ), pixmap( midTitle ), pixmap( rightTitle ) );

        // Space under the right button group
        const int rightButtonsLeft = tx + tw;
        if ( w - rightCornerWidth > rightButtonsLeft )
            painter->drawTiledPixmap( QRect( rightButtonsLeft, 0, w - rightCornerWidth - rightButtonsLeft, TITLESIZE ), pixmap( rightButtons ) );

        //topRightCorner
        if ( rightCornerWidth > 0 )
            painter->drawPixmap( QRect( w - rightCornerWidth, 0, rightCornerWidth, TITLESIZE ), pixmap( topRightCorner ) );

        const QString c = window()->caption();
        const QFont font = settings()->font();
        const QFontMetrics fm( font );
        const int captionWidth = fm.horizontalAdvance( c );

        // shadow text effect
        if ( s.useShdText )
        {
            if ( captionBufferDirty_ )
                updateCaptionBuffer();

            const QImage &shadowImg = active ? activeShadowImg_ : inActiveShadowImg_;
            const QPoint offset = active ? s.activeShdTextOffset : s.inActiveShdTextOffset;
            const int captionHeight = fm.height();
            int dx = 0;
            const int dy = ( TITLESIZE / 2 ) - ( captionHeight / 2 ) + offset.y();

            if ( s.titleAlign == Qt::AlignLeft || captionWidth > titleR.width() )
                dx = tx + 1 + offset.x();
            else if ( s.titleAlign == Qt::AlignHCenter )
                dx = tx + ( tw / 2 ) - ( ( captionWidth + ( MARGIN * 2 ) ) / 2 ) + offset.x();
            else if ( s.titleAlign == Qt::AlignRight )
                dx = tx + tw - ( captionWidth + ( MARGIN * 2 ) ) - 1 + offset.x();

            painter->drawImage( dx, dy, shadowImg, 0, 0, qMin( shadowImg.width(), titleR.width() ), shadowImg.height() );
        }

        //draw titleR text
        painter->setFont( font );
        painter->setPen( window()->color( colorGroup, KDecoration3::ColorRole::Foreground ) );

        Qt::Alignment titleAlign = s.titleAlign;
        if ( captionWidth > titleR.width() )
            titleAlign = Qt::AlignLeft;

        const QRect textRect( tx + MARGIN, ty, tw - ( MARGIN * 2 ), th );
        painter->drawText( textRect, titleAlign | Qt::AlignVCenter, fm.elidedText( c, Qt::ElideRight, textRect.width() ) );
    }

    // draw frames
    if ( !window()->isShaded() || s.showBtmBorder )
    {
        if ( !noBorders )
        {
            const int BOTTOMFRAMESIZE = qRound( borderBottom() );

            if ( h - TITLESIZE - BOTTOMFRAMESIZE > 0 )
            {
                const int leftRightFramesHeight = h - TITLESIZE - BOTTOMFRAMESIZE;

                //left frame
                drawVerticalTiled( painter, QRect( 0, TITLESIZE, theme_->leftFrameSize, leftRightFramesHeight ),
                                   pixmap( topLeftFrame ), pixmap( midLeftFrame ), pixmap( bottomLeftFrame ) );

                //rightFrame
                drawVerticalTiled( painter, QRect( w - theme_->rightFrameSize, TITLESIZE, theme_->rightFrameSize, leftRightFramesHeight ),
                                   pixmap( topRightFrame ), pixmap( midRightFrame ), pixmap( bottomRightFrame ) );
            }

            // bottom frame
            drawHorizontalTiled( painter, QRect( 0, h - BOTTOMFRAMESIZE, w, BOTTOMFRAMESIZE ),
                                 pixmap( leftBottomFrame ), pixmap( midBottomFrame ), pixmap( rightBottomFrame ) );
        }
    }
    else
    {
        painter->setPen( QColor( 70, 70, 70 ) );
        painter->drawLine( 0, h - 1, w, h - 1 );
    }

    leftButtons_->paint( painter, repaintArea );
    rightButtons_->paint( painter, repaintArea );

    painter->restore();
}

//////////////////////////////////////////////////////////////////////////////
// updateCaptionBuffer()
// ------------
// Render the shadow for the window title
void Decoration::updateCaptionBuffer()
{
    const Settings &s = theme_->settings();
    const QString c = window()->caption();
    const QFont font = settings()->font();
    const QFontMetrics fm( font );
    const int captionWidth = fm.horizontalAdvance( c );
    const int captionHeight = fm.height();

    // prepare the shadow
    QImage textImage( captionWidth + ( MARGIN * 2 ), captionHeight, QImage::Format_ARGB32 ); // 2*2 px shadow space
    textImage.fill( 0 );
    QPainter painter( &textImage );
    painter.setFont( font );
    painter.setPen( Qt::white );
    painter.drawText( textImage.rect(), Qt::AlignCenter, c );
    painter.end();

    ShadowEngine se;
    activeShadowImg_ = se.makeShadow( textImage, s.activeShadowColor );
    inActiveShadowImg_ = se.makeShadow( textImage, s.inActiveShadowColor );

    captionBufferDirty_ = false;
}


#include "deKoratorclient.moc"
