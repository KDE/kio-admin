// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "dbustypes.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>

QDBusArgument &operator<<(QDBusArgument &argument, const KIO::UDSEntry &entry)
{
    QBuffer buffer;
    buffer.open(QBuffer::WriteOnly);
    QDataStream stream(&buffer);
    stream << entry;

    argument.beginStructure();
    argument << buffer.data();
    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, KIO::UDSEntry &entry)
{
    QByteArray data;

    argument.beginStructure();
    argument >> data;
    argument.endStructure();

    QDataStream stream(data);
    stream >> entry;

    return argument;
}
