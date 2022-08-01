// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "getcommand.h"

#include <KIO/TransferJob>

GetCommand::GetCommand(const QUrl &url,
                       const QString &remoteService,
                       const QDBusObjectPath &objectPath,
                       QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
{
}

void GetCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::get(m_url);
    setParent(job);
    connect(job, &KIO::TransferJob::data, this, [this](KIO::Job *, const QByteArray &blob) {
        sendSignal(&GetCommand::data, blob);
    });
    connect(job, &KIO::TransferJob::mimeTypeFound, this, [this](KIO::Job *, const QString &mimetype) {
        sendSignal(&GetCommand::mimeTypeFound, mimetype);
    });
    connect(job, &KIO::TransferJob::result, this, [this, job](KJob *) {
        sendSignal(&GetCommand::result, job->error(), job->errorString());
    });
}
