/*
 * deKoratorthemes.h - select theme from (and manage) list of installed deKorator themes
 *
 * Copyright (c) 2009 Christoph Feck <christoph@maxiom.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef DEKORATORTHEMES_H
#define DEKORATORTHEMES_H

#include "kthemeselector.h"

#include <KAboutData>


class deKoratorThemes : public KThemeSelector
{
    Q_OBJECT

    public:
        enum ViewMode {
            FullPreviews,
            NamesOnly,
            // number of view modes
            ViewModes
        };

    public:
        explicit deKoratorThemes(QWidget *parent = nullptr);
        ~deKoratorThemes() override;

    public:
        QString installTheme(const QUrl &themeUrl) override;

    protected:
        bool isValidTheme(const QString &localPath) const override;
        QString themeName(const QString &localPath) const override;

        int viewModes() const override;
        QString viewModeLabel(int viewMode) const override;
        void paintThemeItem(QPainter *painter, const QStyleOptionViewItem *option,
                            const QString &localPath, int viewMode) const override;
        QSize sizeHintThemeItem(const QStyleOptionViewItem *option,
                                const QString &localPath, int viewMode) const override;

    private:
        bool installThemes(const QStringList &themes, const QString &archiveName);
        QStringList findThemeDirs(const QString &archiveName);

    private:
        KAboutData aboutData;
};

#endif

