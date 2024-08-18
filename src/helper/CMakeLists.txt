# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_admin\")

add_executable(kio-admin-helper
    main.cpp
    auth.cpp
    busobject.cpp
    chmodcommand.cpp
    chowncommand.cpp
    copycommand.cpp
    delcommand.cpp
    file.cpp
    getcommand.cpp
    listdircommand.cpp
    mkdircommand.cpp
    putcommand.cpp
    renamecommand.cpp
    statcommand.cpp
    ../dbustypes.cpp
    ../kioadmin_debug.cpp)
    
target_link_libraries(kio-admin-helper
    KF${QT_MAJOR_VERSION}::KIOCore
    PolkitQt${QT_MAJOR_VERSION}-1::Core
    Qt::Core
    Qt::DBus)
install(TARGETS kio-admin-helper DESTINATION ${KDE_INSTALL_LIBEXECDIR_KF})

install(FILES org.kde.kio.admin.conf DESTINATION ${KDE_INSTALL_DBUSDIR}/system.d)
install(FILES org.kde.kio.admin.policy DESTINATION ${KDE_INSTALL_DATADIR}/polkit-1/actions)
configure_file(org.kde.kio.admin.service.in ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kio.admin.service)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kio.admin.service DESTINATION ${KDE_INSTALL_DBUSSYSTEMSERVICEDIR})
