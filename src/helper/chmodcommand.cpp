// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "chmodcommand.h"

#include <KIO/SimpleJob>

ChmodCommand::ChmodCommand(const QUrl &url,
                           int permissions,
                           const QString &remoteService,
                           const QDBusObjectPath &objectPath,
                           QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_permissions(permissions)
{
}

void ChmodCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::chmod(m_url, m_permissions);
    setParent(job);
    connect(job, &KIO::SimpleJob::result, this, [this, job](KJob *) {
        sendSignal(&ChmodCommand::result, job->error(), job->errorString());
    });
}
