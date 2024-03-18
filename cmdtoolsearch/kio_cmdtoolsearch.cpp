/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_cmdtoolsearch.h"
#include "cmdtool.h"
#include "cmdtool_manager.h"
#include "kio_cmdtoolsearch_debug.h"

#include "config.h"

#include <KIO/FileCopyJob>
#include <KIO/ListJob>
#include <KIO/StatJob>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryFile>
#include <QUrl>
#include <QUrlQuery>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.cmdtoolsearch" FILE "cmdtoolsearch.json")
};

CmdToolSearchProtocol::CmdToolSearchProtocol(const QByteArray &pool, const QByteArray &app)
    : QObject()
    , WorkerBase("search", pool, app)
{
}

KIO::WorkerResult CmdToolSearchProtocol::stat(const QUrl &url)
{
    KIO::UDSEntry uds;
    uds.reserve(9);
    uds.fastInsert(KIO::UDSEntry::UDS_ACCESS, 0700);
    uds.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    uds.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    uds.fastInsert(KIO::UDSEntry::UDS_ICON_OVERLAY_NAMES, QStringLiteral("baloo"));
    uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_TYPE, i18n("Search Folder"));
    uds.fastInsert(KIO::UDSEntry::UDS_URL, url.url());

    QUrlQuery query(url);
    QString title = query.queryItemValue(QStringLiteral("title"), QUrl::FullyDecoded);
    if (!title.isEmpty()) {
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, title);
        uds.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, title);
    }

    statEntry(uds);
    return KIO::WorkerResult::pass();
}

// Create a UDSEntry for "."
void CmdToolSearchProtocol::listRootEntry()
{
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);
}

KIO::WorkerResult CmdToolSearchProtocol::listDir(const QUrl &url)
{
    listRootEntry();

    const QUrlQuery urlQuery(url);

    const QString search = urlQuery.queryItemValue(QStringLiteral("search"));
    if (search.isEmpty()) {
        return KIO::WorkerResult::pass();
    }
    const bool checkContent = urlQuery.queryItemValue(QStringLiteral("checkContent")) == QLatin1String("yes");
    const QUrl dirUrl = QUrl(urlQuery.queryItemValue(QStringLiteral("url")));

    QString pluginName;
    QString searchPattern = search;

    CmdTool *tool = nullptr;
    CmdToolManager manager;

    if (search.startsWith(QLatin1Char('@'))) {
        const int idx = search.indexOf(QLatin1Char(' '));
        if (idx >= 0) {
            pluginName = search.mid(1, idx - 1);
            searchPattern = search.mid(idx + 1);
        } else {
            pluginName = search.mid(1);
            searchPattern.clear();
        }

        tool = manager.getTool(pluginName);
        if (!tool) {
            qCWarning(KIO_CMDTOOLSEARCH) << "Plugin" << tool->name() << "not found";
            return KIO::WorkerResult::pass();
        } else if (!tool->isAvailable()) {
            qCWarning(KIO_CMDTOOLSEARCH) << "Plugin" << tool->name() << "not available";
            return KIO::WorkerResult::pass();
        }
    } else {
        // Decide default plugin
        if (!dirUrl.isLocalFile()) {
            qCWarning(KIO_CMDTOOLSEARCH) << "Cmdtool can only run on local files, fallback to kio_filenamesearch";
            // FIXME: use filenamesearch
            return KIO::WorkerResult::pass();
        }
        if (checkContent) {
            tool = manager.getDefaultFileContentSearchTool();
        } else {
            tool = manager.getDefaultFileNameSearchTool();
        }
        if (!tool || !tool->isAvailable()) {
            qCWarning(KIO_CMDTOOLSEARCH) << "Default plugin" << tool->name() << "not available, fallback to kio_filenamesearch";
            // FIXME: use filenamesearch
            return KIO::WorkerResult::pass();
        }
    }

    qDebug() << "Running plugin" << tool->name() << "with search pattern" << searchPattern;

    QDir rootDir(dirUrl.toLocalFile());
    connect(tool, &CmdTool::result, [this, rootDir](const QString &filePath) {
        QString fullPath = rootDir.filePath(filePath);
        QUrl url = QUrl::fromLocalFile(fullPath);
        KIO::UDSEntry uds;
        uds.reserve(3);
        uds.fastInsert(KIO::UDSEntry::UDS_NAME, url.fileName());
        uds.fastInsert(KIO::UDSEntry::UDS_URL, url.url());
        uds.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, fullPath);
        listEntry(uds);
    });

    tool->run(rootDir.absolutePath(), searchPattern, checkContent);

    return KIO::WorkerResult::pass();
}

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc != 4) {
        qCDebug(KIO_CMDTOOLSEARCH) << "Usage: kio_cmdtoolsearch protocol domain-socket1 domain-socket2";
        return -1;
    }

    CmdToolSearchProtocol worker(argv[2], argv[3]);
    worker.dispatchLoop();

    return 0;
}

#include "kio_cmdtoolsearch.moc"
