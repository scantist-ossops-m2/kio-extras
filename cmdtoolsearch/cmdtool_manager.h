/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "cmdtool.h"

#include <QMap>
#include <QString>
#include <QStringList>

#include <optional>

class CmdToolManager : public QObject
{
    Q_OBJECT

public:
    CmdToolManager();
    ~CmdToolManager();

    QStringList listAllTools();
    QStringList listAvailableTools();

    std::optional<CmdTool *> getTool(const QString &name);
    std::optional<CmdTool *> getDefaultFileNameSearchTool();
    std::optional<CmdTool *> getDefaultFileContentSearchTool();

private:
    std::optional<CmdTool *> getFirstAvailableToolInList(const QStringList &list);

    QMap<QString, CmdTool *> m_tools;
    QStringList m_defaultFileNameSearchTools;
    QStringList m_defaultFileContentSearchTools;
};
