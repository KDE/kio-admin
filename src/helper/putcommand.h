// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusUnixFileDescriptor>
#include <QEventLoop>
#include <QUrl>

#include <KIO/Job>

#include "busobject.h"

class PutCommand : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.PutCommand")
public:
    explicit PutCommand(const QUrl &url,
                        const QDBusUnixFileDescriptor &fd,
                        int permissions,
                        KIO::JobFlags flags,
                        const QString &remoteService,
                        const QDBusObjectPath &objectPath,
                        QObject *parent = nullptr);

public Q_SLOTS:
    void start();
    void kill();
    void readData();
    void atEnd();

Q_SIGNALS:
    void dataRequest();
    void result(int error, const QString &errorString);

private:
    const QUrl m_url;
    const QDBusUnixFileDescriptor m_fd;
    const int m_permissions;
    const KIO::JobFlags m_flags;

    QByteArray m_newData;
    QFile m_file;
    QEventLoop m_loop;
    bool m_atEnd = false;
};
