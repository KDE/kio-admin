// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "listdircommand.h"

#include <KIO/ListJob>

ListDirCommand::ListDirCommand(const QUrl &url,
                               const QString &remoteService,
                               const QDBusObjectPath &objectPath,
                               QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
{
}

void ListDirCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::listDir(m_url);
    // Since we aren't file: proper we need to ensure that a mimetype is available. Otherwise KIO has a hard time guessing
    // what is going on and can end up without a mimetype.
    job->addMetaData(QStringLiteral("statDetails"), QString::number(KIO::StatDefaultDetails | KIO::StatMimeType));
    setParent(job);
    connect(job, &KIO::ListJob::entries, this, [this](KIO::Job *, const KIO::UDSEntryList &list) {
        sendSignal(&ListDirCommand::entries, list);
    });
    connect(job, &KIO::ListJob::result, this, [this](KJob *job) {
        sendSignal(&ListDirCommand::result, job->error(), job->errorString());
    });
}
