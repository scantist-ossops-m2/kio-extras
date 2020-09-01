// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>

#include <KDirNotify>
#include <KPasswdServerClient>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QScopeGuard>
#include <QUrl>
#include <QElapsedTimer>

#include <smbcontext.h>
#include <smbauthenticator.h>
#include <smb-logsettings.h>

#include <errno.h>

// Frontend implementation in place of slavebase
class Frontend : public SMBAbstractFrontend
{
    KPasswdServerClient m_passwd;
public:
    bool checkCachedAuthentication(KIO::AuthInfo &info) override
    {
        return m_passwd.checkAuthInfo(&info, 0, 0);
    }
};

// Trivial move action wrapper. Moves happen in two subsqeuent events so
// we need to preserve the context across one iteration.
class MoveAction
{
public:
    QUrl from;
    QUrl to;

    bool isComplete() const
    {
        return !from.isEmpty() && !to.isEmpty();
    }
};

// Rate limit modification signals. SMB will send modification actions
// every time we write during a copy to the remote. This is very excessive
// signals spam so we limit the amount of actual emissions to dbus.
// This is done here in the notifier rahter than KIO because we have
// a much easier time telling which urls events are happening on.
class ModificationLimiter
{
    Q_DISABLE_COPY(ModificationLimiter)
public:
    ModificationLimiter() = default;
    ~ModificationLimiter()
    {
        qDeleteAll(m_limiter);
    }

    void notify(const QUrl &url)
    {
        QElapsedTimer *timer = m_limiter.value(url, nullptr);
        if (timer && timer->isValid() && !timer->hasExpired(m_timelimit)) {
            qCDebug(KIO_SMB_LOG) << "  withholding modification signal; timer hasn't expired";
            return;
        }
        if (!timer) {
            // unknown url => make space => insert new timer
            if (m_limiter.size() > m_cap) {
                makeSpace();
            }
            timer = new QElapsedTimer;
            m_limiter.insert(url, timer);
        }
        timer->start();
        OrgKdeKDirNotifyInterface::emitFilesChanged({url});
    }

    // A non-move event occured on this URL. If the url is in the limiter then throw it out to reclaim the memory.
    // A non-move means the url was otherwise transformed which by extension means the modification must have
    // concluded. We do not emit a final change here because the current non-move event would imply a specific change
    // anyway.
    void forget(const QUrl &url)
    {
        for (auto it = m_limiter.begin(); it != m_limiter.end(); ++it) {
            if (it.key() == url) {
                delete it.value();
                m_limiter.erase(it);
                return;
            }
        }
    }

    void makeSpace()
    {
        auto oldestIt = m_limiter.begin();
        for (auto it = m_limiter.begin(); it != m_limiter.end(); ++it) {
            if ((*it)->elapsed() > (*oldestIt)->elapsed()) {
                oldestIt = it;
            }
        }
        delete *oldestIt;
        m_limiter.erase(oldestIt);
    }

private:
    static const int m_timelimit = 8000 /* ms */; // time between modification signals
    // How many urls we'll track concurrently. These may not get cleaned up until the cap is exhausted, so in the
    // interested of minimal memory footprint we'll want to keep the cap low.
    static const int m_cap = 4;
    QHash<const QUrl, QElapsedTimer *> m_limiter;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    // Intentionally not localized. This process isn't meant to be used by humans.
    parser.addPositionalArgument(QStringLiteral("URI"),
                                 QStringLiteral("smb: URI of directory to notify on (smb://host.local/share/dir)"));
    parser.process(app);

    Frontend frontend;
    SMBContext smbcContext(new SMBAuthenticator(frontend));

    struct NotifyContext {
        const QUrl url;
        // Modification happens a lot, rate limit the notifications going through dbus.
        ModificationLimiter modificationLimiter;
    };
    NotifyContext context { QUrl(parser.positionalArguments().at(0)), {} };

    auto notify = [](const struct smbc_notify_callback_action *actions, size_t num_actions, void *private_data) -> int {
        auto *context = static_cast<NotifyContext *>(private_data);

        // Some relevant docs for how this works under the hood
        //   https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fasod/271a36e8-c94b-4527-8735-e884f5504cd9
        //   https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/14f9d050-27b2-49df-b009-54e08e8bf7b5

        qCDebug(KIO_SMB_LOG) << "notifiying for n actions:" << num_actions;

        // Moves are a bit award. They arrive in two subsequent events this object helps us collect the events.
        MoveAction pendingMove;

        // Values @ 2.7.1 FILE_NOTIFY_INFORMATION
        //   https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/634043d7-7b39-47e9-9e26-bda64685e4c9
        for (size_t i = 0; i < num_actions; ++i, ++actions) {
            qCDebug(KIO_SMB_LOG) << "  " << actions->action << actions->filename;
            QUrl url(context->url);
            url.setPath(url.path() + "/" + actions->filename);

            if (actions->action != SMBC_NOTIFY_ACTION_MODIFIED) {
                // If the current action isn't a modification forget a possible pending modification from a previous
                // action.
                // NB: by default every copy is followed by a move from f.part to f
                context->modificationLimiter.forget(url);
            }

            switch (actions->action) {
            case SMBC_NOTIFY_ACTION_ADDED:
                OrgKdeKDirNotifyInterface::emitFilesAdded(context->url /* dir */);
                continue;
            case SMBC_NOTIFY_ACTION_REMOVED:
                OrgKdeKDirNotifyInterface::emitFilesRemoved({url});
                continue;
            case SMBC_NOTIFY_ACTION_MODIFIED:
                context->modificationLimiter.notify(url);
                continue;
            case SMBC_NOTIFY_ACTION_OLD_NAME:
                Q_ASSERT(!pendingMove.isComplete());
                pendingMove.from = url;
                continue;
            case SMBC_NOTIFY_ACTION_NEW_NAME:
                pendingMove.to = url;
                Q_ASSERT(pendingMove.isComplete());
                OrgKdeKDirNotifyInterface::emitFileRenamed(pendingMove.from, pendingMove.to);
                pendingMove = MoveAction();
                continue;
            case SMBC_NOTIFY_ACTION_ADDED_STREAM: Q_FALLTHROUGH();
            case SMBC_NOTIFY_ACTION_REMOVED_STREAM: Q_FALLTHROUGH();
            case SMBC_NOTIFY_ACTION_MODIFIED_STREAM:
                // https://docs.microsoft.com/en-us/windows/win32/fileio/file-streams
                // Streams have no real use for us I think. They sound like proprietary
                // information an application might attach to a file.
                continue;
            }
            qCWarning(KIO_SMB_LOG) << "Unhandled action" << actions->action << "on URL" << url;
        }

        return 0;
    };

    qCDebug(KIO_SMB_LOG) << "notifying on" << context.url.toString();
    const int dh = smbc_opendir(qUtf8Printable(context.url.toString()));
    auto dhClose = qScopeGuard([dh]{ smbc_closedir(dh); });
    if (dh < 0) {
        qCWarning(KIO_SMB_LOG) << "-- Failed to smbc_opendir:" << strerror(errno);
        return 1;
    }
    Q_ASSERT(dh >= 0);

    // Values @ 2.2.35 SMB2 CHANGE_NOTIFY Request
    //   https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-smb2/598f395a-e7a2-4cc8-afb3-ccb30dd2df7c
    // Not subscribing to stream changes see the callback handler for details.
    const int nh = smbc_notify(dh,
                               0 /* not recursive */,
                               SMBC_NOTIFY_CHANGE_FILE_NAME |
                               SMBC_NOTIFY_CHANGE_DIR_NAME |
                               SMBC_NOTIFY_CHANGE_ATTRIBUTES |
                               SMBC_NOTIFY_CHANGE_SIZE |
                               SMBC_NOTIFY_CHANGE_LAST_WRITE |
                               SMBC_NOTIFY_CHANGE_LAST_ACCESS |
                               SMBC_NOTIFY_CHANGE_CREATION |
                               SMBC_NOTIFY_CHANGE_EA |
                               SMBC_NOTIFY_CHANGE_SECURITY,
                               0 /* no eventlooping necessary */, notify, &context);
    if (nh == -1) {
        qCWarning(KIO_SMB_LOG) << "-- Failed to smbc_notify:" << strerror(errno);
        return 2;
    }
    Q_ASSERT(nh == 0);

    return app.exec();
}
