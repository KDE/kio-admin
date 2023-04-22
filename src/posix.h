// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#pragma once

#include <system_error>

#include <QString>

inline QString qstrerror(int err)
{
    return QString::fromStdString(std::system_error(err, std::generic_category()).code().message());
}
