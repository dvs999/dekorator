cmake_minimum_required(VERSION 3.22)

project(dekorator VERSION 0.6.0)

set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.5.0")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

# the theme selector and config code still rely on implicit const char* -> QString casts
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core DBus Gui Widgets)
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Config
    CoreAddons
    I18n
    IconThemes
    KCMUtils
    KIO
    NewStuff
    WidgetsAddons
    XmlGui
)
find_package(KDecoration3 REQUIRED)

add_definitions(-DTRANSLATION_DOMAIN=\"kwin_deKorator\")

add_subdirectory(client)
add_subdirectory(themesStuff)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
