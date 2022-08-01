// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusArgument>

#include <KIO/UDSEntry>

Q_DECLARE_METATYPE(KIO::UDSEntryList)

QDBusArgument &operator<<(QDBusArgument &argument, const KIO::UDSEntry &entry);
const QDBusArgument &operator>>(const QDBusArgument &argument, KIO::UDSEntry &entry);
