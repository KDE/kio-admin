// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "getcommand.h"

#include <climits>

#include <QDBusUnixFileDescriptor>

#include <KIO/TransferJob>

#include "../posix.h"

GetCommand::GetCommand(const QUrl &url, const QDBusUnixFileDescriptor &fd, const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent)
    : BusObject(remoteService, objectPath, parent)
    , m_url(url)
    , m_fd(fd)
{
}

void GetCommand::start()
{
    if (!isAuthorized()) {
        sendErrorReply(QDBusError::AccessDenied);
        return;
    }

    auto file = new QFile(this);
    if (!file->open(m_fd.fileDescriptor(), QIODevice::WriteOnly, QFileDevice::DontCloseHandle)) { // don't close the fd. we don't own it.
        sendErrorReply(QDBusError::Failed, file->errorString());
        return;
    }

    auto job = KIO::get(m_url);
    setParent(job);
    connect(job, &KIO::TransferJob::data, this, [file, this](KIO::Job *job, const QByteArray &blob) {
        if (blob.size() == 0) {
            return;
        }

        // pipes are a bit tricky, if the client reads too slowly we need to repeat our write until the data fits
        // into the pipe buffer
        off_t offset = 0;
        while (offset < blob.size()) {
            const auto block = blob.mid(offset, PIPE_BUF);
            const auto written = write(file->handle(), block.constData(), block.size());
            if (written == -1) {
                const auto err = errno;
                if (err == EAGAIN) {
                    continue;
                }
                const auto message = qstrerror(err);
                qWarning() << Q_FUNC_INFO << message;
                job->kill(KJob::Quietly);
                sendSignal(&GetCommand::result, int(KIO::ERR_CANNOT_READ), message);
                return;
            }
            offset += written;
            file->flush();
            sendSignal(&GetCommand::readData);
        }
    });
    connect(job, &KIO::TransferJob::mimeTypeFound, this, [this](KIO::Job *, const QString &mimetype) {
        sendSignal(&GetCommand::mimeTypeFound, mimetype);
    });
    connect(job, &KIO::TransferJob::result, this, [this, job](KJob *) {
        sendSignal(&GetCommand::result, job->error(), job->errorString());
    });
}

void GetCommand::kill()
{
    doKill();
}
