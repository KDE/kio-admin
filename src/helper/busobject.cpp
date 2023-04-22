// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "busobject.h"

#include <KJob>

#include "auth.h"

BusObject::BusObject(const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent)
    : QObject(parent)
    , m_remoteService(remoteService)
    , m_objectPath(objectPath)
{
}

bool BusObject::isAuthorized()
{
    return ::isAuthorized(this);
}

void BusObject::setParent(KJob *parent)
{
    QObject::setParent(parent);
    m_job = parent;
}

void BusObject::doKill()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    if (m_job) {
        m_job->kill(KJob::Quietly);
    }
}
