// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "auth.h"

#include <QDBusContext>
#include <QDBusMessage>

#include <polkitqt1-agent-session.h>
#include <polkitqt1-authority.h>

bool isAuthorized(QDBusContext *context)
{
    const auto action = QStringLiteral("org.kde.kio.admin.commands");

    auto authority = PolkitQt1::Authority::instance();
    PolkitQt1::Authority::Result result =
        authority->checkAuthorizationSync(action,
                                          PolkitQt1::SystemBusNameSubject(context->message().service()),
                                          PolkitQt1::Authority::AllowUserInteraction);

    if (authority->hasError()) {
        authority->clearError();
        return false;
    }

    switch (result) {
    case PolkitQt1::Authority::Yes:
        return true;
    case PolkitQt1::Authority::Unknown:
    case PolkitQt1::Authority::No:
    case PolkitQt1::Authority::Challenge:
        break;
    }
    return false;
}
