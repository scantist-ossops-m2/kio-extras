/*
 *   SPDX-FileCopyrightText: 2024 Jin Liu <m.liu.jin@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cmdtool.h"
#include "cmdtool_manager.h"

#include <KLocalizedString>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTimer>

namespace Options
{

static QCommandLineOption list()
{
    static QCommandLineOption o{QStringLiteral("list"), i18n("list all tools")};
    return o;
}

static QCommandLineOption check()
{
    static QCommandLineOption o{QStringLiteral("check"), i18nc("Do not translate <tool>", "check if <tool> is available"), QStringLiteral("tool")};
    return o;
}

static QCommandLineOption run()
{
    static QCommandLineOption o{QStringLiteral("run"), i18nc("Do not translate <tool>", "run <tool>"), QStringLiteral("tool")};
    return o;
}

} // namespace Options

Q_GLOBAL_STATIC_WITH_ARGS(QTextStream, cout, (stdout))
Q_GLOBAL_STATIC_WITH_ARGS(QTextStream, cerr, (stderr))

class CmdToolApp : public QCoreApplication
{
    Q_OBJECT

public:
    CmdToolApp(int &argc, char **argv, QCommandLineParser *parser)
        : QCoreApplication(argc, argv)
        , m_parser(parser)
    {
        QTimer::singleShot(0, this, &CmdToolApp::runMain);
    }

private Q_SLOTS:
    void runMain()
    {
        CmdToolManager manager;

        if (m_parser->isSet(Options::list())) {
            // list all tools
            for (auto i : manager.listAllTools()) {
                CmdTool *tool = manager.getTool(i);
                *cout << i << '\t' << tool->path() << '\t' << tool->isAvailable() << '\n';
            }
            exit(0);
        }

        if (m_parser->isSet(Options::check())) {
            QString name = m_parser->value(Options::check());
            CmdTool *tool = manager.getTool(name);
            if (!tool) {
                *cerr << i18n("Tool %1 not found", name) << '\n';
                exit(1);
            }
            bool available = tool->isAvailable();
            if (available) {
                *cout << i18n("Tool %1 is available", name) << '\n';
                exit(0);
            } else {
                *cout << i18n("Tool %1 is not available", name) << '\n';
                exit(1);
            }
        }

        if (m_parser->isSet(Options::run())) {
            QString name = m_parser->value(Options::run());
            CmdTool *tool = manager.getTool(name);
            if (!tool) {
                *cerr << i18n("Tool %1 not found", name) << '\n';
                exit(1);
            }
            QString searchDir = m_parser->positionalArguments().at(0);
            QString searchPattern = m_parser->positionalArguments().at(1);
            bool checkContent = false;
            connect(tool, &CmdTool::result, [](QString pathStr) {
                *cout << pathStr << '\n';
            });
            bool success = tool->run(searchDir, searchPattern, checkContent);
            exit(success ? 0 : 1);
        }
    }

private:
    QCommandLineParser *m_parser;
};

int main(int argc, char **argv)
{
    QCommandLineParser parser;
    CmdToolApp app(argc, argv, &parser);
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("cmdtool"));

    const QString description = i18n("CmdTool");
    const auto version = QStringLiteral("1.0");

    app.setApplicationVersion(version);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.setApplicationDescription(description);
    parser.addOptions({Options::list(), Options::check(), Options::run()});
    parser.addPositionalArgument(QStringLiteral("search_dir"), i18n("The search dir"));
    parser.addPositionalArgument(QStringLiteral("search_term"), i18n("The search term"));
    parser.process(app);

    // at least one operation should be specified
    if (!parser.isSet(Options::list()) && !parser.isSet(Options::check()) && !parser.isSet(Options::run())) {
        parser.showHelp(0);
    }
    return app.exec();
}

#include "cmdtool_main.moc"