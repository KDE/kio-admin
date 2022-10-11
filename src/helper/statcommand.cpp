// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "statcommand.h"

StatCommand::StatCommand(const QUrl &url, const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
{
}

void StatCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::stat(m_url);
    setParent(job);
    // Since we aren't file: proper we need to ensure that a mimetype is available. Otherwise KIO has a hard time guessing
    // what is going on and can end up without a mimetype.
    job->addMetaData(QStringLiteral("statDetails"), QString::number(KIO::StatDefaultDetails | KIO::StatMimeType));
    connect(job, &KIO::StatJob::result, this, [this, job](KJob *) {
        if (job->error() == KJob::NoError) {
            sendSignal(&StatCommand::statEntry, job->statResult());
        }
        sendSignal(&StatCommand::result, job->error(), job->errorString());
    });
}
