// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "renamecommand.h"

#include <KIO/SimpleJob>

RenameCommand::RenameCommand(const QUrl &src,
                             const QUrl &dst,
                             KIO::JobFlags flags,
                             const QString &remoteService,
                             const QDBusObjectPath &objectPath,
                             QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_src(src)
    , m_dst(dst)
    , m_flags(flags)
{
}

void RenameCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::rename(m_src, m_dst, m_flags);
    setParent(job);
    connect(job, &KIO::SimpleJob::result, this, [this, job](KJob *) {
        sendSignal(&RenameCommand::result, job->error(), job->errorString());
    });
}
