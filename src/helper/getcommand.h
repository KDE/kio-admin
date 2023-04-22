// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusUnixFileDescriptor>
#include <QUrl>

#include "busobject.h"

#include <KJob>

class GetCommand : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.GetCommand")
public:
    explicit GetCommand(const QUrl &url,
                        const QDBusUnixFileDescriptor &fd,
                        const QString &remoteService,
                        const QDBusObjectPath &objectPath,
                        QObject *parent = nullptr);

public Q_SLOTS:
    void start();
    void kill();

Q_SIGNALS:
    void readData();
    void result(int error, const QString &errorString);
    void mimeTypeFound(const QString &mimetype);

private:
    const QUrl m_url;
    const QDBusUnixFileDescriptor m_fd;
};
