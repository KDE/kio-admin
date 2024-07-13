// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <atomic>
#include <chrono>
#include <optional>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDebug>
#include <polkitqt1-agent-session.h>
#include <polkitqt1-authority.h>

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
using namespace std::chrono_literals;

namespace
{
constexpr auto killPollInterval = 200ms;

/**
 * After a user made a choice we want to act accordingly. However, the user might change their
 * opinion after a while. So we need to ask them again even though they have already made a
 * decision in the past.
 */
constexpr std::chrono::duration durationForWhichWeHonorAUsersChoice{5s};
} // namespace

/**
 * @brief A representation of an authorization request (e.g. a password prompt) used for book-keeping.
 *
 * This class is only used for read requests (e.g. receiving information for a file or reading the
 * contents of a directory) for now because read authorization requests are created in rapid
 * succession and spam the end user with password prompts. On the contrary, write authorization
 * requests are generally explicitly triggered by users and therefore rare enough that we do not
 * need to shield users from them.
 */
class ReadAuthorizationRequest
{
public:
    enum class Result { Allowed, Denied };

    void setResult(const QDBusMessage &reply)
    {
        Q_ASSERT(reply.type() == QDBusMessage::ReplyMessage || reply.type() == QDBusMessage::ErrorMessage);
        setResult(reply.type() == QDBusMessage::ReplyMessage ? Result::Allowed : Result::Denied);
    }
    void setResult(Result result)
    {
        Q_ASSERT(!m_result.has_value());
        m_completionTime = std::chrono::system_clock::now();
        m_result = result;
    };

    /** @returns whether the request was successful, denied, or didn't get a response yet. */
    [[nodiscard]] std::optional<Result> result() const
    {
        return m_result;
    };

    /**
     * Two ReadAuthorizationRequest are considered similar if the end user is likely to want the
     * same outcome for both requests. This is the case when authorization for similar actions is
     * requested within a short time frame.
     * This method is commutative (meaning (a.isSimilar(b) == b.isSimilar(a)) is always true).
     *
     * @note Currently this method doesn't care if the read requests are for similar items or not.
     */
    [[nodiscard]] bool isSimilarTo(ReadAuthorizationRequest other) const
    {
        // We do not care about the details if we can compare the results of the requests.
        if (m_result.has_value() && other.m_result.has_value()) {
            return m_result.value() == other.m_result.value();
        }
        if (m_completionTime && other.m_creationTime < m_completionTime.value() + std::chrono::duration<int>(durationForWhichWeHonorAUsersChoice)) {
            return true;
        }
        if (other.m_completionTime && m_creationTime < other.m_completionTime.value() + std::chrono::duration<int>(durationForWhichWeHonorAUsersChoice)) {
            return true;
        }
        if (std::chrono::duration_cast<std::chrono::seconds>(other.m_creationTime - m_creationTime) < durationForWhichWeHonorAUsersChoice) {
            return true;
        }
        return false;
    };

    /**
     * A ReadAuthorizationRequest is considered still relevant if the opinion of the end user on this request is unlikely to have changed since it completed.
     * We always consider it relevant if it has not even completed yet.
     */
    [[nodiscard]] bool isStillRelevant() const
    {
        if (!m_completionTime) {
            // This wasn't even completed yet, so it must still be relevant.
            return true;
        }
        return std::chrono::system_clock::now() - m_completionTime.value() < durationForWhichWeHonorAUsersChoice;
    };

private:
    /** The time of construction of this object. */
    std::chrono::system_clock::time_point m_creationTime = std::chrono::system_clock::now();
    /** The point in time in which this request received its only and final result. */
    std::optional<std::chrono::system_clock::time_point> m_completionTime;

    /** Whether this request was successful, denied, or didn't get a response yet. */
    std::optional<Result> m_result;
};

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

    // Start the eventloop but check every couple milliseconds if the worker was
    // aborted, if that is the case quit the loop.
    void execLoop(QEventLoop &loop)
    {
        QTimer timer;
        timer.setInterval(killPollInterval);
        timer.setSingleShot(false);
        connect(&timer, &QTimer::timeout, &timer, [this, &loop] {
            if (wasKilled()) {
                loop.quit();
            }
        });
        timer.start();
        loop.exec();
    }

    // Variant of execLoop which additionally will forward the kill order to
    // the command object. This allows us to cancel long running operations
    // such as get().
    template<typename Iface>
    void execLoopWithTerminatingIface(QEventLoop &loop, Iface &iface)
    {
        QTimer timer;
        timer.setInterval(killPollInterval);
        timer.setSingleShot(false);
        connect(
            &timer,
            &QTimer::timeout,
            &timer,
            [this, &loop, &iface] {
                if (wasKilled()) {
                    iface.kill();
                    loop.quit();
                }
            },
            Qt::QueuedConnection);
        timer.start();
        loop.exec();
    }

    [[nodiscard]] WorkerResult toFailure(const QDBusMessage &msg)
    {
        qWarning() << msg.errorName() << msg.errorMessage();
        switch (QDBusError(msg).type()) {
        case QDBusError::AccessDenied:
            return WorkerResult::fail(ERR_ACCESS_DENIED, msg.errorMessage());
        default:
            break;
        }
        return WorkerResult::fail();
    }

    /** @returns true if \a request is considered more important than what was remembered previously. false otherwise. */
    bool considerRemembering(ReadAuthorizationRequest request)
    {
        auto previousRequest = s_previousReadAuthorisationRequest.load();
        while (previousRequest && !previousRequest->isStillRelevant()) {
            // The previousRequest is somewhat useless. We forget about it.
            if (s_previousReadAuthorisationRequest.compare_exchange_strong(previousRequest, std::nullopt)) {
                // s_previousReadAuthorisationRequest was successfully set to std::nullopt
                break;
            }
        }

        if (!request.isStillRelevant()) {
            return false;
        }

        // If s_previousReadAuthorisationRequest is empty, assign the current request to it.
        std::optional<ReadAuthorizationRequest> nulloptRequest = std::nullopt;
        if (s_previousReadAuthorisationRequest.compare_exchange_strong(nulloptRequest, request)) {
            return true;
        }

        if ((!previousRequest || !previousRequest->result()) && request.result()) {
            // The previousRequest doesn't have a result yet which makes it less useful than a request with a result that isStillRelevant().
            if (s_previousReadAuthorisationRequest.compare_exchange_strong(previousRequest, request)) {
                return true;
            }
            // s_previousReadAuthorisationRequest changed from another thread since we loaded() its value at the start of this method.
            // We could now either try again or give up. It's easier to give up and there isn't much harm in that so let's do that.
            return false;
        }

        if (!previousRequest || !previousRequest->isSimilarTo(request)) {
            // The previous and the current request are different. Currently only remembering of a singular request is implemented, so now we do not care about
            // the older request anymore and replace it with the current request.
            if (s_previousReadAuthorisationRequest.compare_exchange_strong(previousRequest, request)) {
                return true;
            }
            // s_previousReadAuthorisationRequest changed from another thread since we loaded() its value at the start of this method.
            // We could now either try again or give up. It's easier to give up and there isn't much harm in that so let's do that.
            return false;
        }

        // The request we are considering to remember is about as useful to us as the value s_previousReadAuthorisationRequest currently holds.
        // The older request is supposed to be completed first though, which makes it generally more interesting to us.
        return false;
    }

    /**
     * @returns std::nullopt if there hasn't been a previous similar request somewhat recently.
     *          If there has been a similar request, the result of that request is returned.
     *          If the previous similar request is itself still awaiting its result, this method will make this thread sleep until we have a result.
     */
    [[nodiscard]] std::optional<ReadAuthorizationRequest::Result> resultOfPreviousRequestSimilarTo(ReadAuthorizationRequest request)
    {
        Q_ASSERT(!request.result()); // There is no point wondering about previous results when there is already a result for this request.

        const bool firstReadRequestInAWhile = considerRemembering(request);

        if (!firstReadRequestInAWhile) {
            auto previousRequest = s_previousReadAuthorisationRequest.load();
            while (previousRequest && previousRequest->isSimilarTo(request) && !previousRequest->result()) {
                std::this_thread::sleep_for(durationForWhichWeHonorAUsersChoice / 2);
                previousRequest = s_previousReadAuthorisationRequest.load();
            }
            if (previousRequest && previousRequest->isSimilarTo(request)) {
                return previousRequest->result();
            }
        }
        return std::nullopt;
    }

    WorkerResult listDir(const QUrl &url) override
    {
        ReadAuthorizationRequest thisRequest;
        const auto resultOfPreviousSimilarRequest = resultOfPreviousRequestSimilarTo(thisRequest);
        if (resultOfPreviousSimilarRequest && resultOfPreviousSimilarRequest == ReadAuthorizationRequest::Result::Denied) {
            return WorkerResult::fail();
        }

        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("listDir"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        thisRequest.setResult(reply);

        considerRemembering(thisRequest);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
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

        execLoopWithTerminatingIface(loop, iface);

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
            return toFailure(reply);
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

        execLoop(loop);
        return m_result;
    }

    WorkerResult read(KIO::filesize_t size) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->read(size);
        execLoop(loop);
        return m_result;
    }

    WorkerResult write(const QByteArray &data) override
    {
        qDebug() << Q_FUNC_INFO;
        Q_ASSERT(!m_pendingWrite.has_value());
        m_pendingWrite = data.size();
        m_file->write(data);
        execLoop(loop);
        return m_result;
    }

    WorkerResult seek(KIO::filesize_t offset) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->seek(offset);
        execLoop(loop);
        return m_result;
    }

    WorkerResult truncate(KIO::filesize_t size) override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->truncate(size);
        execLoop(loop);
        return m_result;
    }

    WorkerResult close() override
    {
        qDebug() << Q_FUNC_INFO;
        m_file->close();
        execLoop(loop);
        return m_result;
    }

    WorkerResult put(const QUrl &url, int permissions, JobFlags flags) override
    {
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("put"));
        request << url.toString() << permissions << static_cast<int>(flags);
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
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

        execLoopWithTerminatingIface(loop, iface);
        return m_result;
    }

    WorkerResult stat(const QUrl &url) override
    {
        ReadAuthorizationRequest thisRequest;
        const auto resultOfPreviousSimilarRequest = resultOfPreviousRequestSimilarTo(thisRequest);
        if (resultOfPreviousSimilarRequest && resultOfPreviousSimilarRequest == ReadAuthorizationRequest::Result::Denied) {
            return WorkerResult::fail();
        }

        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("stat"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        thisRequest.setResult(reply);

        considerRemembering(thisRequest);

        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        QDBusConnection::systemBus()
            .connect(serviceName(), path, QStringLiteral("org.kde.kio.admin.StatCommand"), QStringLiteral("statEntry"), this, SLOT(entry(KIO::UDSEntry)));

        OrgKdeKioAdminStatCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminStatCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        QDBusConnection::systemBus().call(
            QDBusMessage::createMethodCall(serviceName(), path, QStringLiteral("org.kde.kio.admin.StatCommand"), QStringLiteral("start")));

        execLoop(loop);

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
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();
        qDebug() << path;

        OrgKdeKioAdminCopyCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminCopyCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
        return m_result;
    }

    WorkerResult get(const QUrl &url) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("get"));
        request << url.toString();
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
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

        execLoopWithTerminatingIface(loop, iface);
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
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminDelCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminDelCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
        return m_result;
    }

    WorkerResult mkdir(const QUrl &url, int permissions) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("mkdir"));
        request << url.toString() << permissions;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminMkdirCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminMkdirCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
        return m_result;
    }

    WorkerResult rename(const QUrl &src, const QUrl &dest, JobFlags flags) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("rename"));
        request << src.toString() << dest.toString() << static_cast<int>(flags);
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminRenameCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminRenameCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
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
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminChmodCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminChmodCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
        return m_result;
    }

    WorkerResult chown(const QUrl &url, const QString &owner, const QString &group) override
    {
        qDebug() << Q_FUNC_INFO;
        auto request = QDBusMessage::createMethodCall(serviceName(), servicePath(), serviceInterface(), QStringLiteral("chown"));
        request << url.toString() << owner << group;
        auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            return toFailure(reply);
        }
        const auto path = reply.arguments().at(0).value<QDBusObjectPath>().path();

        OrgKdeKioAdminChownCommandInterface iface(serviceName(), path, QDBusConnection::systemBus(), this);
        connect(&iface, &OrgKdeKioAdminChownCommandInterface::result, this, &AdminWorker::result);
        iface.start();

        execLoop(loop);
        return m_result;
    }

    // WorkerResult setModificationTime(const QUrl &url, const QDateTime &mtime) override
    // {
    //     qDebug() << Q_FUNC_INFO;
    //     return WorkerResult::pass();
    // }

    WorkerResult special(const QByteArray &data) override
    {
        int tmp;
        QDataStream stream(data);

        stream >> tmp;
        switch (tmp) {
        case 1: { // Wait until the authorization has expired and only return then.
            auto authority = PolkitQt1::Authority::instance();
            PolkitQt1::UnixProcessSubject process(QCoreApplication::applicationPid());
            PolkitQt1::Authority::Result result =
                authority->checkAuthorizationSync(QStringLiteral("org.kde.kio.admin.commands"), process, PolkitQt1::Authority::AllowUserInteraction);
            while (result == PolkitQt1::Authority::Yes && !wasKilled()) {
                std::this_thread::sleep_for(5s);
                result = authority->checkAuthorizationSync(QStringLiteral("org.kde.kio.admin.commands"), process, PolkitQt1::Authority::None);
            }
            return WorkerResult::pass();
        }
        default:
            break;
        }
        return WorkerResult::pass();
    }

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

    inline static std::atomic<std::optional<ReadAuthorizationRequest>> s_previousReadAuthorisationRequest{};
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
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
#else

class KIOPluginFactory : public KIO::WorkerFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.admin" FILE "admin.json")
public:
    std::unique_ptr<KIO::WorkerBase> createWorker(const QByteArray &pool, const QByteArray &app) override
    {
        qRegisterMetaType<KIO::UDSEntryList>("KIO::UDSEntryList");
        qDBusRegisterMetaType<KIO::UDSEntryList>();
        qRegisterMetaType<KIO::UDSEntry>("KIO::UDSEntry");
        qDBusRegisterMetaType<KIO::UDSEntry>();
        return std::make_unique<AdminWorker>(QByteArrayLiteral("admin"), pool, app);
    }
};

#endif

#include "worker.moc"
