// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "file.h"

#include <KIO/FileJob>

File::File(const QUrl &url,
           QIODevice::OpenMode openMode,
           const QString &remoteService,
           const QDBusObjectPath &objectPath,
           QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_openMode(openMode)
{
}

void File::open()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    m_job = KIO::open(m_url, m_openMode);
    setParent(m_job);
    connect(m_job, &KIO::FileJob::open, this, [this] { sendSignal(&File::opened); });
    connect(m_job, &KIO::FileJob::fileClosed, this, [this] { sendSignal(&File::closed); });
    connect(m_job, &KIO::FileJob::data, this, [this](KIO::Job *, const QByteArray &blob) {
        sendSignal(&File::data, blob);
    });
    connect(m_job, &KIO::FileJob::truncated, this, [this](KIO::Job *, qulonglong length) {
        sendSignal(&File::truncated, length);
    });
    connect(m_job, &KIO::FileJob::written, this, [this](KIO::Job *, qulonglong length) {
        sendSignal(&File::written, length);
    });
    connect(m_job, &KIO::FileJob::position, this, [this](KIO::Job *, qulonglong offset) {
        sendSignal(&File::positionChanged, offset);
    });
    connect(m_job, &KIO::FileJob::mimeTypeFound, this, [this](KIO::Job *, const QString &mimeType) {
        sendSignal(&File::mimeTypeFound, mimeType);
    });
    connect(m_job, &KIO::Job::result, this, [this](KJob *job) {
        sendSignal(&File::result, job->error(), job->errorString());
    });
}

void File::read(qulonglong size)
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }
    m_job->read(size);
}

void File::write(const QByteArray &data)
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }
    m_job->write(data);
}

void File::close()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }
    m_job->close();
}

void File::seek(qulonglong offset)
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }
    m_job->seek(offset);
}

void File::truncate(qulonglong length)
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }
    m_job->truncate(length);
}

qulonglong File::size()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return 0;
    }
    return m_job->size();
}
