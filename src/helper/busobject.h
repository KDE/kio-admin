// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

#include <QDBusConnection>
#include <QDBusContext>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QMetaMethod>

#include "../kioadmin_debug.h"

class KJob;

class BusObject : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    void setParent(QObject *parent) = delete;

protected:
    BusObject(const QString &remoteService, const QDBusObjectPath &objectPath, QObject *parent = nullptr);

    template<typename PointerToMemberFunction, typename... Args>
    void sendSignal(PointerToMemberFunction signal, Args &&...args)
    {
        auto method = QMetaMethod::fromSignal(signal);
        Q_ASSERT(method.isValid());
        auto message = QDBusMessage::createTargetedSignal(m_remoteService,
                                                          m_objectPath.path(),
                                                          QLatin1String(metaObject()->classInfo(metaObject()->indexOfClassInfo("D-Bus Interface")).value()),
                                                          QLatin1String(method.name()));
        ((message << QVariant::fromValue(args)), ...);
        QDBusConnection::systemBus().send(message);
    }

    bool isAuthorized();
    void setParent(KJob *parent);
    void doKill();

private:
    const QString m_remoteService;
    const QDBusObjectPath m_objectPath;

    KJob *m_job = nullptr;
};
