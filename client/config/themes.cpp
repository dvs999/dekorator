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



#include "deKoratorthemes.h"

#include <KIO/StoredTransferJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KTar>

#include <QApplication>
#include <QDir>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <memory>


static QString localThemesDir()
{
    return QStandardPaths::writableLocation( QStandardPaths::GenericDataLocation ) + QLatin1String( "/deKorator/themes/" );
}

static const KArchiveDirectory *subDirectory( const KArchiveDirectory *dir, const QString &name )
{
    const KArchiveEntry *entry = dir->entry( name );
    return entry && entry->isDirectory() ? static_cast<const KArchiveDirectory *>( entry ) : nullptr;
}

QString deKoratorThemes::installTheme(const QUrl &themeURL)
{
    // themeTmpFile contains the name of the downloaded file
    QString themeTmpFile;
    std::unique_ptr<QTemporaryFile> tmpFile;
    QWidget *parent_ = this;

    if ( themeURL.isLocalFile() )
    {
        themeTmpFile = themeURL.toLocalFile();
        if ( !QFile::exists( themeTmpFile ) )
        {
            KMessageBox::error( parent_, i18n( "Unable to find the deKorator theme archive %1.", themeURL.toDisplayString() ) );
            return QString();
        }
    }
    else
    {
        KIO::StoredTransferJob *job = KIO::storedGet( themeURL );
        KJobWidgets::setWindow( job, parent_ );
        // keep the file name, so that the archive type can be detected
        tmpFile = std::make_unique<QTemporaryFile>( QDir::tempPath() + QLatin1String( "/deKorator-XXXXXX-" ) + themeURL.fileName() );
        if ( !job->exec() || !tmpFile->open() || tmpFile->write( job->data() ) != job->data().size() )
        {
            KMessageBox::error( parent_, i18n( "Unable to download deKorator theme archive;\n"
                    "please check that address %1 is correct.", themeURL.toDisplayString() ) );
            return QString();
        }
        tmpFile->close();
        themeTmpFile = tmpFile->fileName();
    }

    //
    QStringList themesNames = findThemeDirs( themeTmpFile );
    if ( themesNames.isEmpty() )
    {
        QString invalidArch( i18n( "The file is not a valid deKorator theme archive." ) );
        KMessageBox::error( parent_, invalidArch );
        return QString();
    }

    if ( !installThemes( themesNames, themeTmpFile ) )
    {
        //FIXME: make me able to know what is wrong....
        // QStringList instead of bool?
        QString somethingWrong =
            i18n( "A problem occurred during the installation process; "
                  "however, most of the themes in the archive have been installed" );
        KMessageBox::error( parent_, somethingWrong );
    }

    rescanThemes();

    return localThemesDir() + themesNames.at( 0 );
}

bool deKoratorThemes::installThemes( const QStringList &themes, const QString &archiveName )
{
    bool everythingOk = true;
    const QString themesDir = localThemesDir();

    if ( !QDir().mkpath( themesDir ) ) {
        return false;
    }
    //
    QProgressDialog progressDiag( this );
    progressDiag.setAutoClose( true );
    progressDiag.setRange( 0, themes.count() );
    progressDiag.show();

    KTar archive( archiveName );
    if ( !archive.open( QIODevice::ReadOnly ) )
        return false;
    QApplication::processEvents();

    const KArchiveDirectory* rootDir = archive.directory();

    for ( const QString &theme : themes )
    {
        progressDiag.setLabelText(
            i18n( "<qt>Installing <strong>%1</strong> theme</qt>", theme )
            );
        QApplication::processEvents();

        if ( progressDiag.wasCanceled() )
            break;

        const KArchiveDirectory *currentTheme = subDirectory( rootDir, theme );
        if ( !currentTheme || !currentTheme->copyTo( themesDir + theme ) )
        {
            // we tell back that something went wrong, but try to install as much
            // as possible
            everythingOk = false;
            continue;
        }

        progressDiag.setValue( progressDiag.value() + 1 );
    }

    archive.close();
    return everythingOk;
}

QStringList deKoratorThemes::findThemeDirs( const QString &archiveName )
{
    QStringList foundThemes;

    KTar archive( archiveName );
    if ( !archive.open( QIODevice::ReadOnly ) )
        return foundThemes;
    const KArchiveDirectory* themeDir = archive.directory();

    // iterate all the dirs looking for deco and buttons directories
    const QStringList entries = themeDir->entries();
    for ( const QString &name : entries )
    {
        const KArchiveDirectory *subDir = subDirectory( themeDir, name );
        if ( subDir
            && ( ( subDir->entry( QStringLiteral( "deco" ) ) || subDir->entry( QStringLiteral( "Deco" ) ) )
            && ( subDir->entry( QStringLiteral( "buttons" ) ) || subDir->entry( QStringLiteral( "Buttons" ) ) ) ) )
        {
            foundThemes.append( subDir->name() );
        }
    }

    archive.close();

    return foundThemes;
}
