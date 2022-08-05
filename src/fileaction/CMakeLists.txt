# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

kcoreaddons_add_plugin(fileaction SOURCES fileaction.cpp INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kfileitemaction")
set_target_properties(fileaction PROPERTIES OUTPUT_NAME "kio-admin")
target_link_libraries(fileaction KF5::I18n KF5::KIOWidgets)
