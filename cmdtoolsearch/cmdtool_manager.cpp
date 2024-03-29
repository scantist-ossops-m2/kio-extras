/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool_manager.h"
#include "config.h"

CmdToolManager::CmdToolManager()
{
    QList<QString> searchDirs;
    searchDirs.append(QString::fromUtf8(qgetenv("HOME")) + QStringLiteral("/.local/lib/libexec/cmdtoolsearch"));
    searchDirs.append(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch"));

    for (const QString &searchDir : searchDirs) {
        QDir dir(searchDir);
        if (!dir.exists()) {
            continue;
        }

        for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (m_tools.contains(entry)) {
                continue;
            }

            QDir toolDir(dir.absoluteFilePath(entry));
            CmdTool *tool = new CmdTool(toolDir.canonicalPath());
            if (tool->isValid()) {
                m_tools.insert(entry, tool);
            } else {
                delete tool;
            }
        }
    }
}

CmdToolManager::~CmdToolManager()
{
    for (auto i : m_tools.values()) {
        delete i;
    }
}

QStringList CmdToolManager::listAllTools()
{
    auto result = m_tools.keys();
    result.sort();
    return result;
}

QStringList CmdToolManager::listAvailableTools()
{
    QStringList result;
    for (auto i : m_tools.keys()) {
        if (m_tools[i]->isAvailable()) {
            result.append(i);
        }
    }
    result.sort();
    return result;
}

CmdTool *CmdToolManager::getTool(const QString &name)
{
    if (m_tools.contains(name)) {
        return m_tools[name];
    } else {
        return nullptr;
    }
}

CmdTool *CmdToolManager::getDefaultFileNameSearchTool()
{
    return getTool(QStringLiteral("default_file_name_search"));
}

CmdTool *CmdToolManager::getDefaultFileContentSearchTool()
{
    return getTool(QStringLiteral("default_file_content_search"));
}

#include "cmdtool_manager.moc"