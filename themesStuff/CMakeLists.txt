# Ship the default theme so the decoration works without first installing one.
file(ARCHIVE_EXTRACT
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/Default-theme.tar.bz2
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Default-theme
    DESTINATION ${KDE_INSTALL_DATADIR}/deKorator/themes
)
