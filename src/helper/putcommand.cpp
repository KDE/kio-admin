// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "putcommand.h"

PutCommand::PutCommand(const QUrl &url, int permissions, KIO::JobFlags flags, const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_permissions(permissions)
    , m_flags(flags)
{
}

void PutCommand::start()
{
    qDebug() << Q_FUNC_INFO;
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto job = KIO::put(m_url, m_permissions, m_flags);
    setParent(job);
    connect(job, &KIO::TransferJob::dataReq, this, [this](KIO::Job *, QByteArray &data) {
        qDebug() << Q_FUNC_INFO << "data request";
        sendSignal(&PutCommand::dataRequest);
        m_loop.exec();
        data = m_newData;
    });
    connect(job, &KIO::TransferJob::result, this, [this, job](KJob *) {
        qDebug() << Q_FUNC_INFO << "result" << job->errorString();
        sendSignal(&PutCommand::result, job->error(), job->errorString());
    });
}

void PutCommand::data(const QByteArray &data)
{
    qDebug() << Q_FUNC_INFO;
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    m_newData = data;
    m_loop.quit();
}

void PutCommand::kill()
{
    doKill();
}
