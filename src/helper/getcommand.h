// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QUrl>

#include "busobject.h"

class GetCommand : public BusObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin.GetCommand")
public:
    explicit GetCommand(const QUrl &url,
                        const QString &remoteService,
                        const QDBusObjectPath &objectPath,
                        QObject *parent = nullptr);

public Q_SLOTS:
    void start();

Q_SIGNALS:
    void data(const QByteArray &blob);
    void result(int error, const QString &errorString);
    void mimeTypeFound(const QString &mimetype);

private:
    QUrl m_url;
};
