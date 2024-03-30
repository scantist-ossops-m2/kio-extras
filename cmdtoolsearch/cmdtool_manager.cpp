/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool_manager.h"
#include "config.h"

static void loadDefaults(QStringList &list, const QString &name, const QStringList &searchDirs)
{
    for (const QString &i : searchDirs) {
        QDir dir(i);
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine();
                line = line.trimmed();
                if (!line.isEmpty() && !line.startsWith(QLatin1Char('#'))) {
                    list.append(line);
                }
            }
            return;
        }
    }
}

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

    loadDefaults(m_defaultFileNameSearchTools, QStringLiteral("default_file_name_search"), searchDirs);
    loadDefaults(m_defaultFileContentSearchTools, QStringLiteral("default_file_content_search"), searchDirs);
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

std::optional<CmdTool *> CmdToolManager::getTool(const QString &name)
{
    if (m_tools.contains(name)) {
        return m_tools[name];
    } else {
        return std::nullopt;
    }
}

std::optional<CmdTool *> CmdToolManager::getDefaultFileNameSearchTool()
{
    return getFirstAvailableToolInList(m_defaultFileNameSearchTools);
}

std::optional<CmdTool *> CmdToolManager::getDefaultFileContentSearchTool()
{
    return getFirstAvailableToolInList(m_defaultFileContentSearchTools);
}

std::optional<CmdTool *> CmdToolManager::getFirstAvailableToolInList(const QStringList &list)
{
    for (const QString &name : list) {
        std::optional<CmdTool *> tool = getTool(name);
        if (tool && (*tool)->isAvailable()) {
            return tool;
        }
    }
    return nullptr;
}

#include "cmdtool_manager.moc"