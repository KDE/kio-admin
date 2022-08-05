// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include <QAction>
#include <QDebug>

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
    }

    QList<QAction *> actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget) override
    {
        qDebug() << fileItemInfos.isLocal();
        if (!fileItemInfos.isLocal()) {
            return {};
        }

        auto action = new QAction(i18nc("@action", "Open as Administrator"), parentWidget);
        action->setIcon(QIcon::fromTheme(QStringLiteral("gcr-key")));
        QList<QUrl> urls;
        const auto items = fileItemInfos.items();
        for (const auto &item : items) {
            auto url = item.url();
            url.setScheme(QStringLiteral("admin"));
            if (item.isDir() && fileItemInfos.items().size() <= 1) {
                // Descend into the directory immediately if it is the only selected item...
                url.setPath(url.path() + QLatin1String("/"));
            }
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
