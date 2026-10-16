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


#ifndef DEKORATORCLIENT_H
#define DEKORATORCLIENT_H

#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButton>
#include <KDecoration3/DecorationButtonGroup>

#include <QColor>
#include <QHash>
#include <QImage>
#include <QList>
#include <QPixmap>
#include <QPoint>
#include <QRegion>
#include <QVariant>

#include <memory>

class QTimer;


namespace DeKorator
{

enum decoType {
    topLeftCorner = 0,
    leftButtons,
    leftTitle,
    midTitle,
    rightTitle,
    rightButtons,
    topRightCorner,
    topLeftFrame,
    midLeftFrame,
    bottomLeftFrame,
    leftBottomFrame,
    midBottomFrame,
    rightBottomFrame,
    topRightFrame,
    midRightFrame,
    bottomRightFrame,
    decoCount
};

enum buttonTypeAll {
    restore = 0,
    help,
    max,
    min,
    close,
    sticky,
    stickydown,
    above,
    abovedown,
    below,
    belowdown,
    shade,
    shadedown,
    menu,
    tabClose,
    buttonTypeAllCount
};

enum buttonState {
    regular = 0,
    hover,
    press,
    buttonStateCount
};

enum WindowActivationState {
    WindowActive = 0,
    WindowInactive,
    WindowActivationStateCount
};

enum maskType {
    topLeftMask = 0,
    topMidMask,
    topRightMask,
    bottomLeftMask,
    bottomMidMask,
    bottomRightMask,
    maskCount
};

// Settings /////////////////////////////////////////////////////////////////
// everything read from kwindeKoratorrc

struct Settings
{
    void read();
    bool operator==( const Settings &other ) const = default;

    // misc
    Qt::Alignment titleAlign = Qt::AlignHCenter;
    bool useMenuImage = false;
    bool ignoreAppIconCol = false;
    bool showBtmBorder = false;
    bool showMaximizedBorders = false;
    bool useShdText = false;
    QPoint activeShdTextOffset;
    QColor activeShadowColor = QColor( 150, 150, 150 );
    QPoint inActiveShdTextOffset;
    QColor inActiveShadowColor = QColor( 150, 150, 150 );
    QPoint btnShift;

    // colors
    bool colorizeActFrames = false;
    bool colorizeActButtons = false;
    bool colorizeInActFrames = false;
    bool colorizeInActButtons = false;
    bool useAnimation = false;
    QString animationType = QStringLiteral( "Intensity" );
    int animSteps = 5;
    int animInterval = 30;
    bool keepAnimating = false;
    QString buttonHoverType = QStringLiteral( "To Gray" );
    float effectAmount = 0.5;
    QColor activeHighlightColor = QColor( 150, 150, 150 );
    QColor inActiveHighlightColor = QColor( 150, 150, 150 );
    QString decoColorize = QStringLiteral( "Liquid Method" );
    QString buttonsColorize = QStringLiteral( "Liquid Method" );

    // buttons
    bool useCustomButtonsColors = false;
    bool customColorsActiveButtons = false;
    bool customColorsInActiveButtons = false;
    QColor cusBtnCol[ buttonTypeAllCount ];

    // paths
    QString framesPath;
    QString buttonsPath;
    QString masksPath;
    bool useMasks = true;
};

// Theme ////////////////////////////////////////////////////////////////////
// the images of the configured theme, shared by all decorations
// (this replaces the KDE 4 DeKoratorFactory)

class Theme
{
public:
    static std::shared_ptr<Theme> self();

    // re-read the configuration, returns true if anything changed
    bool reconfigure();

    const Settings &settings() const { return settings_; }
    bool isValid() const { return titleSize > 0; }

    QPixmap decoPixmap( decoType part, bool active, const QColor &titleBarColor );
    QImage buttonImage( buttonTypeAll type, buttonState state, bool active, const QColor &buttonColor );
    QSize buttonSize( buttonTypeAll type ) const;
    QRegion maskedRegion( const QSize &size, bool withBottom ) const;

    static QImage colorizedImage( const QImage &image, const QColor &color, const QString &colorizeMethod );

    int titleSize = 0;
    int leftFrameSize = 0;
    int rightFrameSize = 0;
    int bottomFrameSize = 0;
    int topLeftCornerWidth = 0;
    int topRightCornerWidth = 0;

private:
    void loadPixmaps();
    void determineSizes();

    bool loaded_ = false;
    Settings settings_;
    QImage decoImage_[ decoCount ][ WindowActivationStateCount ];
    QImage buttonImage_[ buttonTypeAllCount ][ buttonStateCount ][ WindowActivationStateCount ];
    QRegion mask_[ maskCount ];
    QSize maskSize_[ maskCount ];

    // recolored images, keyed by the color they were colorized with
    QHash<QRgb, QList<QPixmap>> decoCache_[ WindowActivationStateCount ];
    QHash<QRgb, QList<QImage>> buttonCache_[ WindowActivationStateCount ];
};

// Decoration ///////////////////////////////////////////////////////////////

class Decoration : public KDecoration3::Decoration
{
    Q_OBJECT
public:
    explicit Decoration( QObject *parent = nullptr, const QVariantList &args = QVariantList() );
    ~Decoration() override;

    bool init() override;
    void paint( QPainter *painter, const QRectF &repaintArea ) override;

    Theme *theme() const { return theme_.get(); }
    bool hideBorders() const;
    QColor titleBarColor( bool active ) const;
    QColor buttonColor( bool active ) const;

private Q_SLOTS:
    void reconfigure();
    void recreateButtons();
    void updateLayout();
    void updateButtonsGeometry();

private:
    void updateCaptionBuffer();
    QRect titleRect() const;
    QRegion maskRegion() const;

    std::shared_ptr<Theme> theme_;
    KDecoration3::DecorationButtonGroup *leftButtons_ = nullptr;
    KDecoration3::DecorationButtonGroup *rightButtons_ = nullptr;
    bool captionBufferDirty_ = true;
    QImage activeShadowImg_, inActiveShadowImg_;
    mutable QRegion mask_;
    mutable QSize maskSize_;
    mutable bool maskWithBottom_ = false;
};

// Button ///////////////////////////////////////////////////////////////////

class Button : public KDecoration3::DecorationButton
{
    Q_OBJECT
public:
    Button( KDecoration3::DecorationButtonType type, Decoration *decoration, QObject *parent = nullptr );

    static KDecoration3::DecorationButton *create( KDecoration3::DecorationButtonType type, KDecoration3::Decoration *decoration, QObject *parent );

    void paint( QPainter *painter, const QRectF &repaintArea ) override;
    void updateSize();

private Q_SLOTS:
    void hoverChange( bool hovered );
    void animate();

private:
    buttonTypeAll glyph() const;
    QImage chooseRightHoverEffect( const QImage &image, const QString &effect ) const;
    Decoration *deco() const;

    QTimer *animTmr_;
    int animProgress_ = 0;
    int step_ = 0;
};

} // namespace DeKorator

#endif // DEKORATORCLIENT_H
