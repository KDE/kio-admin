// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QWidget>

#include <KAbstractFileItemActionPlugin>
#include <KFileItem>
#include <KFileItemListProperties>
#include <KIO/OpenFileManagerWindowJob>
#include <KLocalizedString>
#include <KPluginFactory>

class Plugin : public KAbstractFileItemActionPlugin
{
    Q_OBJECT
public:
    explicit Plugin(QObject *parent = nullptr, const QVariantList &args = {})
        : KAbstractFileItemActionPlugin(parent)
    {
        Q_UNUSED(args);
    }

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override
    {
        qDebug() << fileItemInfos.isLocal();
        if (!fileItemInfos.isLocal()) {
            return {};
        }
        qDebug() << "actions!!!";
        const auto items = fileItemInfos.items();
        if (std::all_of(items.cbegin(), items.cend(), [](const KFileItem &item) {
                auto url = item.mostLocalUrl();
                return url.path().startsWith(QDir::homePath());
            })) {
            return {};
        }

        auto action = new QAction(QIcon::fromTheme(QStringLiteral("yast-auth-client")),
                                  i18nc("@action", "Open as Administrator"),
                                  parentWidget);
        QList<QUrl> urls;
        for (const auto &item : items) {
            auto url = item.url();
            url.setScheme(QStringLiteral("admin"));
            if (item.isDir() && fileItemInfos.items().size() <= 1 && url.path() != QLatin1String("/")) {
                // Descend into the directory immediately if it is the only selected item...
                url.setPath(url.path() + QLatin1String("/"));
            }
            qDebug() << "urly!!!" << url;
            urls << url;
        }
        connect(action, &QAction::triggered, this, [urls, this] {
            auto job = new KIO::OpenFileManagerWindowJob(this);
            job->setHighlightUrls(urls);
            job->start();
        });
        return {action};
    }

};

K_PLUGIN_CLASS_WITH_JSON(Plugin, "fileaction.json")

#include "fileaction.moc"
