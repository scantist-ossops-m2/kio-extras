/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/WorkerBase>

#include <QUrl>

#include <queue>
#include <set>

class CmdToolSearchProtocol : public QObject, public KIO::WorkerBase
{
    Q_OBJECT

public:
    CmdToolSearchProtocol(const QByteArray &pool, const QByteArray &app);

    KIO::WorkerResult stat(const QUrl &url) override;
    KIO::WorkerResult listDir(const QUrl &url) override;

private:
    void listRootEntry();
    void searchDir(const QUrl &dirUrl, const QRegularExpression &regex, bool searchContents, std::set<QString> &iteratedDirs, std::queue<QUrl> &pendingDirs);
    KIO::WorkerResult runPlugin(const QUrl &url);
};
