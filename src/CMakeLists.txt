# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_admin\")

add_subdirectory(fileaction)
add_subdirectory(helper)

macro(generate_and_use_interfaces)
    foreach(interface ${ARGV})
        qt_generate_dbus_interface(helper/${interface}.h org.kde.kio.admin.${interface}.xml OPTIONS -A)
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/org.kde.kio.admin.${interface}.xml PROPERTIES NO_NAMESPACE true)
        qt_add_dbus_interface(admin_SRCS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kio.admin.${interface}.xml interface_${interface})
    endforeach()
endmacro()

set(admin_SRCS)
generate_and_use_interfaces(
    chmodcommand
    chowncommand
    copycommand
    delcommand
    file
    getcommand
    listdircommand
    mkdircommand
    putcommand
    renamecommand
    statcommand)

ecm_qt_declare_logging_category(admin  HEADER kioadmin_debug.h IDENTIFIER KIOADMIN_LOG CATEGORY_NAME org.kde.kio.admin)

kcoreaddons_add_plugin(admin SOURCES  kioadmin_debug.cpp worker.cpp dbustypes.cpp ${admin_SRCS} INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kio")
target_link_libraries(admin
    PUBLIC KF${QT_MAJOR_VERSION}::KIOCore
    PRIVATE atomic
    PolkitQt${QT_MAJOR_VERSION}-1::Core
    Qt::Core
    Qt::DBus)
set_target_properties(admin PROPERTIES OUTPUT_NAME "admin")
set_target_properties(admin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf${QT_MAJOR_VERSION}/kio")
