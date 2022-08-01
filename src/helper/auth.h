// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#pragma once

class QDBusContext;

bool isAuthorized(QDBusContext *context);

#undef Q_EMIT
#define Q_EMIT #error "don't use emit directly use BusObject sendSignal"
