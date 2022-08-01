// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "copycommand.h"

#include <KIO/CopyJob>

CopyCommand::CopyCommand(const QUrl &src,
                         const QUrl &dst,
                         int permissions,
                         KIO::JobFlags flags,
                         const QString &remoteService,
                         const QDBusObjectPath &objectPath,
                         QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_src(src)
    , m_dst(dst)
    , m_permissions(permissions)
    , m_flags(flags)
{
}

void CopyCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::copy(m_src, m_dst, m_flags);
    setParent(job);
    connect(job, &KIO::CopyJob::result, this, [this, job](KJob *) {
        sendSignal(&CopyCommand::result, job->error(), job->errorString());
    });
}
