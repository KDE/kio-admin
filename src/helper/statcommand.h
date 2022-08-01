// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QUrl>

#include <KIO/Job>

#include "busobject.h"

class StatCommand : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.StatCommand")
public:
    explicit StatCommand(const QUrl &url,
                         const QString &remoteService,
                         const QDBusObjectPath &objectPath,
                         QObject *parent = nullptr);

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void statEntry(const KIO::UDSEntry &entry);
    void result(int error, const QString &errorString);

private:
    QUrl m_url;
};
