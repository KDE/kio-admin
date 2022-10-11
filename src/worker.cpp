// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <optional>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDebug>

#include <KIO/WorkerBase>
#include <KIO/WorkerFactory>

#include "dbustypes.h"
#include "interface_chmodcommand.h"
#include "interface_chowncommand.h"
#include "interface_copycommand.h"
#include "interface_delcommand.h"
#include "interface_file.h"
#include "interface_getcommand.h"
#include "interface_listdircommand.h"
#include "interface_mkdircommand.h"
#include "interface_putcommand.h"
#include "interface_renamecommand.h"
#include "interface_statcommand.h"

using namespace KIO;

class AdminWorker : public QObject, public WorkerBase
{
    Q_OBJECT
public:
    using WorkerBase::WorkerBase;

    static QString serviceName()
    {
        return QStringLiteral("org.kde.kio.admin");
    }

    static QString servicePath()
    {
        return QStringLiteral("/");
    }

    static QString serviceInterface()
    {
        return QStringLiteral("org.kde.kio.admin");
    }

    WorkerResult listDir(const QUrl &url) override
    {
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("listDir"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();
        qDebug() << path;

        OrgKdeKioAdminListDirCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminListDirCommandInterface::result, this, &AdminWorker::result);

        QDBusConnection::systemBus().connect(serviceName(),
                                             path,
                                             QStringLiteral("org.kde.kio.admin.ListDirCommand"),
                                             QStringLiteral("entries"),
                                             this,
                                             SLOT(entries(KIO::UDSEntryList)));

        iface.start();

        loop.exec();

        QDBusConnection::systemBus().disconnect(serviceName(),
                                                path,
                                                QStringLiteral("org.kde.kio.admin.ListDirCommand"),
                                                QStringLiteral("entries"),
                                                this,
                                                SLOT(entries(KIO::UDSEntryList)));
        return m_result;
    }

    WorkerResult open(const QUrl &url, QIODevice::OpenMode mode) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("file"));
        request << url.toString() << (int)mode;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        m_file = std::make_unique<OrgKdeKioAdminFileInterface>(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::opened, this, [this] {
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::written, this, [this](qulonglong length) {
            written(length);
            Q_ASSERT(m_pendingWrite.has_value());
            m_pendingWrite.emplace(m_pendingWrite.value() - length);
            if (m_pendingWrite.value() == 0) {
                loop.quit();
            }
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::data, this, [this](const QByteArray &blob) {
            data(blob);
            loop.quit();
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::positionChanged, this, [this](qulonglong offset) {
            position(offset);
            loop.quit();
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::truncated, this, [this](qulonglong length) {
            truncated(length);
            loop.quit();
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::mimeTypeFound, this, [this](const QString &mimetype) {
            mimeType(mimetype);
            loop.quit();
            result(0, {});
        });
        connect(m_file.get(), &OrgKdeKioAdminFileInterface::result, this, &AdminWorker::result);
        m_file->open();

        loop.exec();
        return m_result;
    }

    WorkerResult read(KIO::filesize_t size) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->read(size);
        loop.exec();
        return m_result;
    }

    WorkerResult write(const QByteArray &data) override
    {
        qDebug() << Q_FUNC_INFO;
        Q_ASSERT(!m_pendingWrite.has_value());
        m_pendingWrite = data.size();
        m_file->write(data);
        loop.exec();
        return m_result;
    }

    WorkerResult seek(KIO::filesize_t offset) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->seek(offset);
        loop.exec();
        return m_result;
    }

    WorkerResult truncate(KIO::filesize_t size) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->truncate(size);
        loop.exec();
        return m_result;
    }

    WorkerResult close() override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->close();
        loop.exec();
        return m_result;
    }

    WorkerResult put(const QUrl &url, int permissions, JobFlags flags) override
    {
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("put"));
        request << url.toString() << permissions << static_cast<int>(flags);
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminPutCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);

        connect(&iface, &OrgKdeKioAdminPutCommandInterface::dataRequest, this, [this, &iface]() {
            dataReq();
            QByteArray buffer;
            if (const int read = readData(buffer); read < 0) {
                qWarning() << "Failed to read data for unknown reason" << read;
            }
            iface.data(buffer);
        });
        connect(&iface, &OrgKdeKioAdminPutCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult stat(const QUrl &url) override
    {
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("stat"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        QDBusConnection::systemBus()
            .connect(serviceName(), path, QStringLiteral("org.kde.kio.admin.StatCommand"), QStringLiteral("statEntry"), this, SLOT(entry(KIO::UDSEntry)));

        OrgKdeKioAdminStatCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminStatCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        QDBusConnection::systemBus().call(
            QDBusMessage::createMethodCall(serviceName(), path, QStringLiteral("org.kde.kio.admin.StatCommand"), QStringLiteral("start")));

        loop.exec();

        QDBusConnection::systemBus()
            .disconnect(serviceName(), path, QStringLiteral("org.kde.kio.admin.StatCommand"), QStringLiteral("statEntry"), this, SLOT(entry(KIO::UDSEntry)));

        return m_result;
    }

    WorkerResult copy(const QUrl &src, const QUrl &dest, int permissions, JobFlags flags) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("copy"));
        request << src.toString() << dest.toString() << permissions << static_cast<int>(flags);
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();
        qDebug() << path;

        OrgKdeKioAdminCopyCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminCopyCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult get(const QUrl &url) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("get"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();
        qDebug() << path;

        OrgKdeKioAdminGetCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminGetCommandInterface::data, this, [this](const QByteArray &blob) {
            data(blob);
        });
        connect(&iface, &OrgKdeKioAdminGetCommandInterface::mimeTypeFound, this, [this](const QString &mimetype) {
            mimeType(mimetype);
        });
        connect(&iface, &OrgKdeKioAdminGetCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult del(const QUrl &url, bool isFile) override
    {
        Q_UNUSED(isFile);

        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("del"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminDelCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminDelCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult mkdir(const QUrl &url, int permissions) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("mkdir"));
        request << url.toString() << permissions;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminMkdirCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminMkdirCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult rename(const QUrl &src, const QUrl &dest, JobFlags flags) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("rename"));
        request << src.toString() << dest.toString() << static_cast<int>(flags);
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminRenameCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminRenameCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    //  WorkerResult symlink(const QString &target, const QUrl &dest, JobFlags flags) override;

    WorkerResult chmod(const QUrl &url, int permissions) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("chmod"));
        request << url.toString() << permissions;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminChmodCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminChmodCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    WorkerResult chown(const QUrl &url, const QString &owner, const QString &group) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("chown"));
        request << url.toString() << owner << group;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qWarning() << reply.errorName() << reply.errorMessage();
            return WorkerResult::fail();
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminChownCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminChownCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        loop.exec();
        return m_result;
    }

    // WorkerResult setModificationTime(const QUrl &url, const QDateTime &mtime) override
    // {
    //     qDebug() << Q_FUNC_INFO;
    //     return WorkerResult::pass();
    // }

private Q_SLOTS:
    void entry(const KIO::UDSEntry &entry)
    {
        qDebug() << Q_FUNC_INFO << entry;
        statEntry(entry);
    }

    void entries(const KIO::UDSEntryList &list)
    {
        qDebug() << Q_FUNC_INFO;
        listEntries(list);
    }

    void result(int error, const QString &errorString)
    {
        qDebug() << "RESULT" << error << errorString;
        if (error != KJob::NoError) {
            m_result = WorkerResult::fail(error, errorString);
        } else {
            m_result = WorkerResult::pass();
        }
        loop.quit();
    }

private:
    WorkerResult m_result = WorkerResult::pass();
    std::unique_ptr<OrgKdeKioAdminFileInterface> m_file;
    QEventLoop loop;
    std::optional<quint64> m_pendingWrite = std::nullopt;
};

class KIOPluginFactory : public KIO::RealWorkerFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.admin" FILE "admin.json")
public:
    std::unique_ptr<KIO::SlaveBase> createWorker(const QByteArray &pool, const QByteArray &app) override
    {
        Q_UNUSED(pool);
        Q_UNUSED(app);
        return {};
    }

    std::unique_ptr<KIO::WorkerBase> createRealWorker(const QByteArray &pool, const QByteArray &app) override
    {
        qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");
        qDBusRegisterMetaType<KIO::UDSEntryList>();
        qRegisterMetaType<KIO::UDSEntry>("KIO::UDSEntry");
        qDBusRegisterMetaType<KIO::UDSEntry>();
        return std::make_unique<AdminWorker>(QByteArrayLiteral("admin"), pool, app);
    }
};

#include "worker.moc"
