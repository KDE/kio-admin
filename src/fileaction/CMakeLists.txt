# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

kcoreaddons_add_plugin(fileaction SOURCES fileaction.cpp ../kioadmin_debug.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kfileitemaction")
set_target_properties(fileaction PROPERTIES OUTPUT_NAME "kio-admin")
target_link_libraries(fileaction
    KF${QT_MAJOR_VERSION}::I18n
    KF${QT_MAJOR_VERSION}::KIOWidgets
    Qt::Core
    Qt::Gui
    Qt::Widgets)
