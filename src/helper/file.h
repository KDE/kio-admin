// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QIODevice>
#include <QUrl>

#include "busobject.h"

namespace KIO
{
class FileJob;
} // namespace KIO

class File : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.File")
public:
    File(const QUrl &url,
         QIODevice::OpenMode openMode,
         const QString &remoteService,
         const QDBusObjectPath &objectPath,
         QObject *parent = nullptr);

public Q_SLOTS:
    void open();
    void read(qulonglong size);
    void write(const QByteArray &data);
    void close();
    void seek(qulonglong offset);
    void truncate(qulonglong length);
    qulonglong size();

Q_SIGNALS:
    void opened();
    void data(const QByteArray &data);
    void mimeTypeFound(const QString &mimeType);
    void written(qulonglong written);
    void closed();
    void positionChanged(qulonglong offset);
    void truncated(qulonglong length);
    void result(int error, const QString &errorMessage);

private:
    KIO::FileJob *m_job = nullptr;
    QUrl m_url;
    QIODevice::OpenMode m_openMode;
};
