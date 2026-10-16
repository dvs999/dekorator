kcoreaddons_add_plugin(kcm_deKoratordecoration
    SOURCES deKoratorconfig.cc deKoratorthemes.cpp themes.cpp kthemeselector.cpp
    INSTALL_NAMESPACE "org.kde.kdecoration3.kcm"
)
ki18n_wrap_ui(kcm_deKoratordecoration configdialog.ui kthemeselectorwidgets.ui)

target_link_libraries(kcm_deKoratordecoration
    KF6::Archive
    KF6::ConfigGui
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::NewStuffWidgets
    KF6::WidgetsAddons
    KF6::XmlGui
    Qt6::DBus
    Qt6::Widgets
)

install(FILES deKoratorthemes.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
