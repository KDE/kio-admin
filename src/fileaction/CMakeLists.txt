# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

kcoreaddons_add_plugin(fileaction SOURCES fileaction.cpp ../kioadmin_debug.cpp INSTALL_NAMESPACE "kf6/kfileitemaction")
set_target_properties(fileaction PROPERTIES OUTPUT_NAME "kio-admin")
target_link_libraries(fileaction
    KF6::I18n
    KF6::KIOWidgets
    Qt::Core
    Qt::Gui
    Qt::Widgets)
