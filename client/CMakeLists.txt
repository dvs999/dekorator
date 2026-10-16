kcoreaddons_add_plugin(org.kde.deKorator
    SOURCES deKoratorclient.cc shadow.cpp
    INSTALL_NAMESPACE "org.kde.kdecoration3"
)
target_link_libraries(org.kde.deKorator
    KDecoration3::KDecoration
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    KF6::IconThemes
    Qt6::Gui
)

add_subdirectory(config)
