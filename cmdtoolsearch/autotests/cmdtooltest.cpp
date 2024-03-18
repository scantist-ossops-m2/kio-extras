#include <QTest>

#include "../cmdtool.h"
#include "../config.h"

class CmdToolTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIsAvailable()
    {
        CmdTool nonexist(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/nonexist"));
        QVERIFY(!nonexist.isAvailable());

        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/find"));
        QVERIFY(find.isAvailable());
    }

    void testMetadata()
    {
        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/find"));
        QCOMPARE(find.separator(), CmdTool::SEP_NUL);

        CmdTool grep(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/grep"));
        QCOMPARE(grep.separator(), CmdTool::SEP_NEWLINE);
    }

    void testFind()
    {
        CmdTool find(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/find"));
        connect(&find, &CmdTool::result, this, [this](const QString &pathStr) {
            qDebug() << pathStr;
        });
        QVERIFY(find.run(QStringLiteral(KDE_INSTALL_FULL_LIBEXECDIR "/cmdtoolsearch/find"), QStringLiteral("run"), false));
    }
};

QTEST_GUILESS_MAIN(CmdToolTest)

#include "cmdtooltest.moc"
