// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusContext>
#include <QDBusMetaType>

#include <KIO/JobUiDelegateExtension>
#include <KIO/JobUiDelegateFactory>

#include "../dbustypes.h"
#include "auth.h"
#include "busobject.h"
#include "chmodcommand.h"
#include "chowncommand.h"
#include "copycommand.h"
#include "delcommand.h"
#include "file.h"
#include "getcommand.h"
#include "listdircommand.h"
#include "mkdircommand.h"
#include "putcommand.h"
#include "renamecommand.h"
#include "statcommand.h"

static QUrl stringToUrl(const QString &stringUrl)
{
    QUrl url(stringUrl);
    if (url.scheme() == QLatin1String("admin")) {
        url.setScheme(QStringLiteral("file"));
    }
    return url;
}

class Helper : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kio.admin")
public Q_SLOTS:
    QDBusObjectPath listDir(const QString &stringUrl)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/listDir/%1").arg(QString::number(counter)));
        auto command = new ListDirCommand(stringToUrl(stringUrl), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath stat(const QString &stringUrl)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/stat/%1").arg(QString::number(counter)));
        auto command = new StatCommand(stringToUrl(stringUrl), message().service(), objPath);
        ;
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath get(const QString &stringUrl)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/get/%1").arg(QString::number(counter)));
        auto command = new GetCommand(stringToUrl(stringUrl), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath put(const QString &stringUrl, int permissions, int flags)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/put/%1").arg(QString::number(counter)));
        auto command = new PutCommand(stringToUrl(stringUrl), permissions, KIO::JobFlags(flags), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath copy(const QString &stringUrlSrc, const QString &stringUrlDst, int permissions, int flags)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/copy/%1").arg(QString::number(counter)));
        auto command = new CopyCommand(stringToUrl(stringUrlSrc), stringToUrl(stringUrlDst), permissions, KIO::JobFlags(flags), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath del(const QString &stringUrl)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/del/%1").arg(QString::number(counter)));
        auto command = new DelCommand(stringToUrl(stringUrl), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath mkdir(const QString &stringUrl, int permissions)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/mkdir/%1").arg(QString::number(counter)));
        auto command = new MkdirCommand(stringToUrl(stringUrl), permissions, message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath chmod(const QString &stringUrl, int permissions)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/chmod/%1").arg(QString::number(counter)));
        auto command = new ChmodCommand(stringToUrl(stringUrl), permissions, message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath chown(const QString &stringUrl, const QString &user, const QString &group)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/chown/%1").arg(QString::number(counter)));
        auto command = new ChownCommand(stringToUrl(stringUrl), user, group, message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath rename(const QString &stringUrlSrc, const QString &stringUrlDst, int flags)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/rename/%1").arg(QString::number(counter)));
        auto command = new RenameCommand(stringToUrl(stringUrlSrc), stringToUrl(stringUrlDst), KIO::JobFlags(flags), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

    QDBusObjectPath file(const QString &stringUrl, int openMode)
    {
        if (!isAuthorized()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        static uint64_t counter = 0;
        counter++;
        Q_ASSERT(counter != 0);

        const QDBusObjectPath objPath(QStringLiteral("/org/kde/kio/admin/file/%1").arg(QString::number(counter)));
        auto command = new File(stringToUrl(stringUrl), static_cast<QIODevice::OpenMode>(openMode), message().service(), objPath);
        connection().registerObject(objPath.path(), command, QDBusConnection::ExportAllSlots);
        return objPath;
    }

private:
    bool isAuthorized()
    {
        return ::isAuthorized(this);
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setQuitLockEnabled(false);

    qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");
    qDBusRegisterMetaType<KIO::UDSEntryList>();

    qRegisterMetaType<KIO::UDSEntry>("KIO::UDSEntry");
    qDBusRegisterMetaType<KIO::UDSEntry>();

    KIO::setDefaultJobUiDelegateFactoryV2(nullptr);
    KIO::setDefaultJobUiDelegateExtension(nullptr);

    Helper helper;

    if (!QDBusConnection::systemBus().registerObject(QStringLiteral("/"), &helper, QDBusConnection::ExportAllSlots)) {
        qWarning() << "Failed to register the daemon object" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }
    if (!QDBusConnection::systemBus().registerService(QStringLiteral("org.kde.kio.admin"))) {
        qWarning() << "Failed to register the service" << QDBusConnection::systemBus().lastError().message();
        return 1;
    }

    return app.exec();
}

#include "main.moc"
