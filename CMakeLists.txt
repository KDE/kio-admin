# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

cmake_minimum_required(VERSION 3.22)
project(kio-admin VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)

include(FeatureSummary)

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.96.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    KIO
)

set(QT_MAJOR_VERSION 5)

find_package(PolkitQt${QT_MAJOR_VERSION}-1 REQUIRED)

include(CheckIncludeFile)
include(CMakePackageConfigHelpers)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMMarkNonGuiExecutable)
include(ECMMarkAsTest)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
