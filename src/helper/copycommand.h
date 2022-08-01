// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QUrl>

#include <KIO/Job>

#include "busobject.h"

class CopyCommand : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.CopyCommand")
public:
    explicit CopyCommand(const QUrl &src,
                         const QUrl &dst,
                         int permissions,
                         KIO::JobFlags flags,
                         const QString &remoteService,
                         const QDBusObjectPath &objectPath,
                         QObject *parent = nullptr);

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void result(int error, const QString &errorString);

private:
    const QUrl m_src;
    const QUrl m_dst;
    const int m_permissions;
    const KIO::JobFlags m_flags;
};
