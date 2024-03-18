#pragma once

#include "cmdtool.h"

#include <QMap>
#include <QString>
#include <QStringList>

class CmdToolManager : public QObject
{
    Q_OBJECT

public:
    CmdToolManager();
    ~CmdToolManager();

    QStringList listAllTools();
    QStringList listAvailableTools();

    CmdTool *getTool(const QString &name);
    CmdTool *getDefaultFileNameSearchTool();
    CmdTool *getDefaultFileContentSearchTool();

private:
    QMap<QString, CmdTool *> m_tools;
};
