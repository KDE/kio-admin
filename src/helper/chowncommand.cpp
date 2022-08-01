// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "chowncommand.h"

#include <KIO/SimpleJob>

ChownCommand::ChownCommand(const QUrl &url,
                           const QString &user,
                           const QString &group,
                           const QString &remoteService,
                           const QDBusObjectPath &objectPath,
                           QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_user(user)
    , m_group(group)
{
}

void ChownCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::chown(m_url, m_user, m_group);
    setParent(job);
    connect(job, &KIO::SimpleJob::result, this, [this, job](KJob *) {
        sendSignal(&ChownCommand::result, job->error(), job->errorString());
    });
}
