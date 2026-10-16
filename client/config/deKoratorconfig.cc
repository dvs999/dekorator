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


#include "deKoratorconfig.h"

#include <KColorButton>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QCheckBox>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "deKoratorthemes.h"

K_PLUGIN_CLASS_WITH_JSON( DeKoratorConfig, "kcm_deKoratordecoration.json" )


// The combo boxes store these untranslated keys, in the order of their items
static const char * const colorizeMethods[] = { "Liquid Method", "KDE Method", "Hue Adjustment", nullptr };
static const char * const animationTypes[] = { "Intensity", "Blend", "Solarize", "Fade", "Trashed", "Hue Shift", nullptr };
static const char * const hoverTypes[] = { "To Gray", "Colorize", "DeSaturate", "To Gamma", "Use Image Colors", nullptr };

static void setComboKey( QComboBox *combo, const char * const keys[], const QString &key )
{
    for ( int i = 0; keys[ i ]; ++i )
    {
        if ( key == QLatin1String( keys[ i ] ) )
        {
            combo->setCurrentIndex( i );
            return;
        }
    }
    combo->setCurrentIndex( 0 );
}

static QString comboKey( const QComboBox *combo, const char * const keys[] )
{
    return QLatin1String( keys[ qMax( 0, combo->currentIndex() ) ] );
}


//////////////////////////////////////////////////////////////////////////////
// DeKoratorConfig()
// -------------
// Constructor

DeKoratorConfig::DeKoratorConfig( QObject *parent, const KPluginMetaData &data )
        : KCModule( parent, data ), config_( KSharedConfig::openConfig( QStringLiteral( "kwindeKoratorrc" ) ) ), m_themes( nullptr ), dialog_( nullptr )
{
    // create and show the configuration dialog
    dialog_ = new ConfigDialog( widget() );
    QVBoxLayout *layout = new QVBoxLayout( widget() );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( dialog_ );

    // KWin itself handles double clicks on the menu button now
    dialog_->dblClkCloseChkBox->hide();

    // setup the connections, any change of an option enables the Apply button
    auto changed = [this] { markAsChanged(); };
    for ( QCheckBox *box : dialog_->findChildren<QCheckBox *>() )
        connect( box, &QCheckBox::toggled, this, changed );
    for ( QRadioButton *button : dialog_->findChildren<QRadioButton *>() )
        connect( button, &QRadioButton::toggled, this, changed );
    for ( QGroupBox *box : dialog_->findChildren<QGroupBox *>() )
        if ( box->isCheckable() )
            connect( box, &QGroupBox::toggled, this, changed );
    for ( QSpinBox *box : dialog_->findChildren<QSpinBox *>() )
        connect( box, &QSpinBox::valueChanged, this, changed );
    for ( QComboBox *box : dialog_->findChildren<QComboBox *>() )
        connect( box, &QComboBox::currentIndexChanged, this, changed );
    for ( KColorButton *button : dialog_->findChildren<KColorButton *>() )
        connect( button, &KColorButton::changed, this, changed );

    // themes (added after the connections above, its own widgets are not options)
    m_themes = new deKoratorThemes( dialog_ );
    m_themes->layout()->unsetContentsMargins();
    dialog_->tabWidget2->insertTab( 0, m_themes, i18n( "Themes" ) );
    dialog_->tabWidget2->setCurrentIndex( 0 );
    connect( m_themes, &KThemeSelector::themeSelected, this, changed );
}

//////////////////////////////////////////////////////////////////////////////
// load()
// ------
// Load configuration data

void DeKoratorConfig::load()
{
    config_->reparseConfiguration();
    KConfig *conf = this->config_.data();
    // misc
    KConfigGroup *config_; KConfigGroup groupMisc(conf, "MISC" ); config_ = &groupMisc;

    QString value = config_->readEntry( "TitleAlignment", "AlignHCenter" );
    QRadioButton *button = dialog_->titlealign->findChild<QRadioButton *>( value );
    if ( button ) button->setChecked( true );
    dialog_->useMenuImageChkBox->setChecked( config_->readEntry( "UseMenuImage", false ) );
	dialog_->ignoreAppIcnCol->setChecked( config_->readEntry( "IgnoreAppIconCol", false ) );
    dialog_->dblClkCloseChkBox->setChecked( config_->readEntry( "DblClkClose", false ) );
    dialog_->showBtmBorderChkBox->setChecked( config_->readEntry( "ShowBtmBorder", false ) );
    dialog_->showMaximizedBordersChkBox->setChecked( config_->readEntry( "ShowMaximizedBorders", false ) );
    dialog_->ignoreMasksChkBox->setChecked( !config_->readEntry( "UseMasks", true ) );
    QColor color = QColor( 150, 150, 150 );
    dialog_->useShdtextChkBox->setChecked( config_->readEntry( "UseShdtext", false ) );
    dialog_->activeShdtextXSpinBox->setValue( config_->readEntry( "ActiveShdtextX", 0 ) );
    dialog_->activeShdtextYSpinBox->setValue( config_->readEntry( "ActiveShdtextY", 0 ) );
    dialog_->activeShdColBtn->setColor( config_->readEntry( "ActiveShadowColor", color ) );
    dialog_->inActiveShdtextXSpinBox->setValue( config_->readEntry( "InActiveShdtextX", 0 ) );
    dialog_->inActiveShdtextYSpinBox->setValue( config_->readEntry( "InActiveShdtextY", 0 ) );
    dialog_->inActiveShdColBtn->setColor( config_->readEntry( "InActiveShadowColor", color ) );
    dialog_->btnShiftXSpinBox->setValue( config_->readEntry( "BtnShiftX", 0 ) );
    dialog_->btnShiftYSpinBox->setValue( config_->readEntry( "BtnShiftY", 0 ) );


    // colors
    KConfigGroup groupColors(conf, "COLORS" ); config_ = &groupColors;

    dialog_->colorizeActFramesChkBox->setChecked( config_->readEntry( "ColorizeActFrames", false ) );
    dialog_->colorizeActButtonsChkBox->setChecked( config_->readEntry( "ColorizeActButtons", false ) );
    dialog_->colorizeInActFramesChkBox->setChecked( config_->readEntry( "ColorizeInActFrames", false ) );
    dialog_->colorizeInActButtonsChkBox->setChecked( config_->readEntry( "ColorizeInActButtons", false ) );
    dialog_->useAnimChkBox->setChecked( config_->readEntry( "UseAnimation", false ) );
    setComboKey( dialog_->animateEfeectsCombo, animationTypes, config_->readEntry( "AnimationType", "Intensity" ) );

    dialog_->stepsSpinBox->setValue( config_->readEntry( "AnimSteps", 5 ) );
    dialog_->intervalSpinBox->setValue( config_->readEntry( "AnimInterval", 30 ) );
    dialog_->KeepAnimateChkBox->setChecked( config_->readEntry( "KeepAnimating", false ) );

    dialog_->activeHighlightClrBtn->setColor( config_->readEntry( "ActiveHighlightColor", color ) );
    dialog_->inActiveHighlightClrBtn->setColor( config_->readEntry( "InActiveHighlightColor", color ) );
    setComboKey( dialog_->hoverTypeCombo, hoverTypes, config_->readEntry( "ButtonsHoverType", "To Gray" ) );
    dialog_->effectsAmount_SpinBox->setValue( config_->readEntry( "EffectAmount", 5 ) );
    setComboKey( dialog_->decoColorizeComboBox, colorizeMethods, config_->readEntry( "DecoColorize", "Liquid Method" ) );
    setComboKey( dialog_->buttonsColorizeComboBox, colorizeMethods, config_->readEntry( "ButtonsColorize", "Liquid Method" ) );
    //  dialog_->buttonsHoverColorizeComboBox->setCurrentText( config_->readEntry( "ButtonsHoverColorize", "Liquid Method" ) );

    // buttons
    KConfigGroup groupButtons(conf, "BUTTONS" ); config_ = &groupButtons;

    dialog_->useCusBtnClrChkBox->setChecked( config_->readEntry( "UseCustomButtonsColors", false ) );
    dialog_->cusColActBtnChkBox->setChecked( config_->readEntry( "customColorsActiveButtons", false ) );
    dialog_->cusColInActBtnChkBox->setChecked( config_->readEntry( "customColorsInActiveButtons", false ) );
    dialog_->closeClrBtn->setColor( config_->readEntry( "CloseButtonColor", color ) );
    dialog_->minClrBtn->setColor( config_->readEntry( "MinButtonColor", color ) );
    dialog_->maxClrBtn->setColor( config_->readEntry( "MaxButtonColor", color ) );
    dialog_->restoreClrBtn->setColor( config_->readEntry( "RestoreButtonColor", color ) );
    dialog_->helpClrBtn->setColor( config_->readEntry( "HelpButtonColor", color ) );
    dialog_->menuClrBtn->setColor( config_->readEntry( "MenuButtonColor", color ) );
    dialog_->stickyClrBtn->setColor( config_->readEntry( "StickyButtonColor", color ) );
    dialog_->stickyDownClrBtn->setColor( config_->readEntry( "StickyDownButtonColor", color ) );
    dialog_->aboveClrBtn->setColor( config_->readEntry( "AboveButtonColor", color ) );
    dialog_->aboveDownClrBtn->setColor( config_->readEntry( "AboveDownButtonColor", color ) );
    dialog_->belowClrBtn->setColor( config_->readEntry( "BelowButtonColor", color ) );
    dialog_->belowDownClrBtn->setColor( config_->readEntry( "BelowDownButtonColor", color ) );
    dialog_->shadeClrBtn->setColor( config_->readEntry( "ShadeButtonColor", color ) );
    dialog_->shadeDownClrBtn->setColor( config_->readEntry( "ShadeDownButtonColor", color ) );

    // path's
    KConfigGroup groupPaths(conf, "PATHS" ); config_ = &groupPaths;

    QString themePath = config_->readEntry( "FramesPath", "" );
    if ( themePath.endsWith( QLatin1String( "/deco" ) ) || themePath.endsWith( QLatin1String( "/Deco") ) ) {
        themePath.chop( 5 );
        // loading is not a change
        QSignalBlocker blocker( m_themes );
        m_themes->setSelectedTheme( themePath );
    }

    setNeedsSave( false );
}

static QString themePath(const QString &localPath, const QString &dirName)
{
    QDir dir(localPath);
    if (dir.exists(dirName)) {
        return (localPath + QLatin1String("/") + dirName);
    }
    QString lowerName = dirName.toLower();
    if (dir.exists(lowerName)) {
        return (localPath + QLatin1String("/") + lowerName);
    }
    return QString();
}

//////////////////////////////////////////////////////////////////////////////
// save()
// ------
// Save configuration data

void DeKoratorConfig::save()
{
    KConfig *conf = this->config_.data();
    // misc
    KConfigGroup *config_; KConfigGroup groupMisc(conf, "MISC" ); config_ = &groupMisc;

    QRadioButton *button = 0;
    QList<QRadioButton *> buttons = dialog_->titlealign->findChildren<QRadioButton *>();
    foreach (QRadioButton *b, buttons) {
        if (b->isChecked()) {
            button = b;
            break;
        }
    }

    if ( button ) config_->writeEntry( "TitleAlignment", QString( button->objectName() ) );
    config_->writeEntry( "UseMenuImage", dialog_->useMenuImageChkBox->isChecked() );
	config_->writeEntry( "IgnoreAppIconCol", dialog_->ignoreAppIcnCol->isChecked() );
    config_->writeEntry( "DblClkClose", dialog_->dblClkCloseChkBox->isChecked() );
    config_->writeEntry( "ShowBtmBorder", dialog_->showBtmBorderChkBox->isChecked() );
    config_->writeEntry( "ShowMaximizedBorders", dialog_->showMaximizedBordersChkBox->isChecked() );
    config_->writeEntry( "UseShdtext", dialog_->useShdtextChkBox->isChecked() );
    config_->writeEntry( "ActiveShdtextX", dialog_->activeShdtextXSpinBox->value() );
    config_->writeEntry( "ActiveShdtextY", dialog_->activeShdtextYSpinBox->value() );
    config_->writeEntry( "ActiveShadowColor", dialog_->activeShdColBtn->color() );
    config_->writeEntry( "InActiveShdtextX", dialog_->inActiveShdtextXSpinBox->value() ); config_->writeEntry( "InActiveShdtextY", dialog_->inActiveShdtextYSpinBox->value() );
    config_->writeEntry( "InActiveShadowColor", dialog_->inActiveShdColBtn->color() );
    config_->writeEntry( "BtnShiftX", dialog_->btnShiftXSpinBox->value() );
    config_->writeEntry( "BtnShiftY", dialog_->btnShiftYSpinBox->value() );


    // colors
    KConfigGroup groupColors(conf, "COLORS" ); config_ = &groupColors;

    config_->writeEntry( "ColorizeActFrames", dialog_->colorizeActFramesChkBox->isChecked() );
    config_->writeEntry( "ColorizeActButtons", dialog_->colorizeActButtonsChkBox->isChecked() );
    config_->writeEntry( "ColorizeInActFrames", dialog_->colorizeInActFramesChkBox->isChecked() );
    config_->writeEntry( "ColorizeInActButtons", dialog_->colorizeInActButtonsChkBox->isChecked() );
    config_->writeEntry( "UseAnimation", dialog_->useAnimChkBox->isChecked() );
    config_->writeEntry( "AnimationType", comboKey( dialog_->animateEfeectsCombo, animationTypes ) );

    config_->writeEntry( "AnimSteps", dialog_->stepsSpinBox->value() );
    config_->writeEntry( "AnimInterval", dialog_->intervalSpinBox->value() );
    config_->writeEntry( "KeepAnimating", dialog_->KeepAnimateChkBox->isChecked() );

    config_->writeEntry( "ButtonsHoverType", comboKey( dialog_->hoverTypeCombo, hoverTypes ) );
    config_->writeEntry( "EffectAmount", dialog_->effectsAmount_SpinBox->value() );
    config_->writeEntry( "ActiveHighlightColor", dialog_->activeHighlightClrBtn->color() );
    config_->writeEntry( "InActiveHighlightColor", dialog_->inActiveHighlightClrBtn->color() );
    config_->writeEntry( "DecoColorize", comboKey( dialog_->decoColorizeComboBox, colorizeMethods ) );
    config_->writeEntry( "ButtonsColorize", comboKey( dialog_->buttonsColorizeComboBox, colorizeMethods ) );
    //  config_->writeEntry( "ButtonsHoverColorize", dialog_->buttonsHoverColorizeComboBox->currentText() );

    // buttons
    KConfigGroup groupButtons(conf, "BUTTONS" ); config_ = &groupButtons;

    config_->writeEntry( "UseCustomButtonsColors", dialog_->useCusBtnClrChkBox->isChecked() );
    config_->writeEntry( "customColorsActiveButtons", dialog_->cusColActBtnChkBox->isChecked() );
    config_->writeEntry( "customColorsInActiveButtons", dialog_->cusColInActBtnChkBox->isChecked() );
    config_->writeEntry( "CloseButtonColor", dialog_->closeClrBtn->color() );
    config_->writeEntry( "MinButtonColor", dialog_->minClrBtn->color() );
    config_->writeEntry( "MaxButtonColor", dialog_->maxClrBtn->color() );
    config_->writeEntry( "RestoreButtonColor", dialog_->restoreClrBtn->color() );
    config_->writeEntry( "HelpButtonColor", dialog_->helpClrBtn->color() );
    config_->writeEntry( "MenuButtonColor", dialog_->menuClrBtn->color() );
    config_->writeEntry( "StickyButtonColor", dialog_->stickyClrBtn->color() );
    config_->writeEntry( "StickyDownButtonColor", dialog_->stickyDownClrBtn->color() );
    config_->writeEntry( "AboveButtonColor", dialog_->aboveClrBtn->color() );
    config_->writeEntry( "AboveDownButtonColor", dialog_->aboveDownClrBtn->color() );
    config_->writeEntry( "BelowButtonColor", dialog_->belowClrBtn->color() );
    config_->writeEntry( "BelowDownButtonColor", dialog_->belowDownClrBtn->color() );
    config_->writeEntry( "ShadeButtonColor", dialog_->shadeClrBtn->color() );
    config_->writeEntry( "ShadeDownButtonColor", dialog_->shadeDownClrBtn->color() );

    // path's
    KConfigGroup groupPaths(conf, "PATHS" ); config_ = &groupPaths;
#if 0
    config_->writeEntry( "FramesPath", dialog_->framesPathKurl->url().path() );
    config_->writeEntry( "ButtonsPath", dialog_->buttonsPathKurl->url().path() );
    config_->writeEntry( "UseMasks", dialog_->useMasks_Chkbox->isChecked() );
    config_->writeEntry( "MasksPath", dialog_->masksPathKurl->url().path() );
#else
    QString path = m_themes->selectedTheme();

    if ( path.isEmpty() ) {
        // TODO no theme selected
    } else {
        config_->writeEntry( "FramesPath", themePath( path, "Deco" ) );
        config_->writeEntry( "ButtonsPath", themePath( path, "Buttons" ) );
        config_->writeEntry( "MasksPath", themePath( path, "Masks" ) );
        config_->writeEntry( "UseMasks", !dialog_->ignoreMasksChkBox->isChecked() );
    }
#endif
    config_->sync();

    // tell KWin to reload the decoration settings
    QDBusMessage message = QDBusMessage::createSignal( QStringLiteral( "/KWin" ), QStringLiteral( "org.kde.KWin" ), QStringLiteral( "reloadConfig" ) );
    QDBusConnection::sessionBus().send( message );

    setNeedsSave( false );
}

//////////////////////////////////////////////////////////////////////////////
// defaults()
// ----------
// Set configuration defaults

void DeKoratorConfig::defaults()
{

    // misc
    QRadioButton * button =
        dialog_->titlealign->findChild<QRadioButton *>( "AlignHCenter" );
    if ( button ) button->setChecked( true );
    dialog_->useMenuImageChkBox->setChecked( false );
	dialog_->ignoreAppIcnCol->setChecked( false );
    dialog_->dblClkCloseChkBox->setChecked( false );
    dialog_->showBtmBorderChkBox->setChecked( false );
    dialog_->showMaximizedBordersChkBox->setChecked( false );
    dialog_->ignoreMasksChkBox->setChecked( false );
    dialog_->useShdtextChkBox->setChecked( false );
    dialog_->activeShdtextXSpinBox->setValue( 0 );
    dialog_->activeShdtextYSpinBox->setValue( 0 );
    dialog_->activeShdColBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->inActiveShdtextXSpinBox->setValue( 0 );
    dialog_->inActiveShdtextYSpinBox->setValue( 0 );
    dialog_->inActiveShdColBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->btnShiftXSpinBox->setValue( 0 );
    dialog_->btnShiftYSpinBox->setValue( 0 );

    // colors
    dialog_->colorizeActFramesChkBox->setChecked( false );
    dialog_->colorizeActButtonsChkBox->setChecked( false );
    dialog_->colorizeInActFramesChkBox->setChecked( false );
    dialog_->colorizeInActButtonsChkBox->setChecked( false );
    dialog_->useAnimChkBox->setChecked( false );
    dialog_->animateEfeectsCombo->setCurrentIndex( 0 );

    dialog_->stepsSpinBox->setValue( 5 );
    dialog_->intervalSpinBox->setValue( 30 );
    dialog_->KeepAnimateChkBox->setChecked( false );

    dialog_->hoverTypeCombo->setCurrentIndex( 0 );
    dialog_->effectsAmount_SpinBox->setValue( 5 );
    dialog_->activeHighlightClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->inActiveHighlightClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->decoColorizeComboBox->setCurrentIndex( 0 );
    dialog_->buttonsColorizeComboBox->setCurrentIndex( 0 );
    //  dialog_->buttonsHoverColorizeComboBox->setCurrentText( "Liquid Method" );

    // buttons
    dialog_->useCusBtnClrChkBox->setChecked( false );
    dialog_->cusColActBtnChkBox->setChecked( false );
    dialog_->cusColInActBtnChkBox->setChecked( false );
    dialog_->closeClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->minClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->maxClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->restoreClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->helpClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->menuClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->stickyClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->stickyDownClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->aboveClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->aboveDownClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->belowClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->belowDownClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->shadeClrBtn->setColor( QColor( 150, 150, 150 ) );
    dialog_->shadeDownClrBtn->setColor( QColor( 150, 150, 150 ) );
}

#include "deKoratorconfig.moc"
