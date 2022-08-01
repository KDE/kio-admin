// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "mkdircommand.h"

#include <KIO/MkdirJob>

MkdirCommand::MkdirCommand(const QUrl &url,
                           int permissions,
                           const QString &remoteService,
                           const QDBusObjectPath &objectPath,
                           QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_permissions(permissions)
{
}

void MkdirCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::mkdir(m_url, m_permissions);
    setParent(job);
    connect(job, &KIO::MkdirJob::result, this, [this, job](KJob *) {
        sendSignal(&MkdirCommand::result, job->error(), job->errorString());
    });
}
