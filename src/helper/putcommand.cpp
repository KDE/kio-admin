// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "putcommand.h"

#include <QThread>

PutCommand::PutCommand(const QUrl &url,
                       const QDBusUnixFileDescriptor &fd,
                       int permissions,
                       KIO::JobFlags flags,
                       const QString &remoteService,
                       const QDBusObjectPath &objectPath,
                       QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_fd(fd)
    , m_permissions(permissions)
    , m_flags(flags)
{
}

void PutCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    if (!m_file.open(m_fd.fileDescriptor(), QIODevice::ReadOnly, QFileDevice::DontCloseHandle)) { // don't close the fd. we don't own it.
        sendErrorReply(QDBusError::Failed, m_file.errorString());
        return;
    }

    auto job = KIO::put(m_url, m_permissions, m_flags);
    setParent(job);
    connect(job, &KIO::TransferJob::dataReq, this, [this](KIO::Job *, QByteArray &data) {
        sendSignal(&PutCommand::dataRequest);
        m_loop.exec();

        if (m_atEnd) {
            data.clear();
            return;
        }

        data = m_file.read(PIPE_BUF);
    });
    connect(job, &KIO::TransferJob::result, this, [this, job](KJob *) {
        sendSignal(&PutCommand::result, job->error(), job->errorString());
    });
}

void PutCommand::readData()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    m_loop.quit();
}

void PutCommand::atEnd()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    m_atEnd = true;
    m_loop.quit();
}

void PutCommand::kill()
{
    doKill();
}
