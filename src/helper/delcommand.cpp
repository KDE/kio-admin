// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "delcommand.h"

#include <KIO/DeleteJob>

DelCommand::DelCommand(const QUrl &url, const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
{
}

void DelCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::del(m_url);
    setParent(job);
    connect(job, &KIO::DeleteJob::result, this, [this, job](KJob *) {
        sendSignal(&DelCommand::result, job->error(), job->errorString());
    });
}
