#include "../platform/applicationpaths.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ApplicationPathsTest final : public QObject
{
    Q_OBJECT

private slots:
    void staysInstalledWithoutMarker();
    void enablesPortableModeFromArgument();
    void enablesPortableModeFromMarker();
};

void ApplicationPathsTest::staysInstalledWithoutMarker()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const ApplicationPaths paths(directory.path());
    QVERIFY(!paths.portableMode());
    QCOMPARE(paths.applicationDirectory(), QDir::cleanPath(directory.path()));
    QVERIFY(paths.settingsFilePath().isEmpty());
    QVERIFY(paths.syncSettingsFilePath().isEmpty());
    QVERIFY(paths.desktopSettingsFilePath().isEmpty());
}

void ApplicationPathsTest::enablesPortableModeFromArgument()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const ApplicationPaths paths(directory.path(), true);
    const QString dataDirectory = QDir(directory.path()).filePath(QStringLiteral("data"));
    QVERIFY(paths.portableMode());
    QCOMPARE(paths.dataDirectory(), dataDirectory);
    QCOMPARE(paths.settingsFilePath(),
             QDir(dataDirectory).filePath(QStringLiteral("settings.ini")));
    QCOMPARE(paths.syncSettingsFilePath(),
             QDir(dataDirectory).filePath(QStringLiteral("sync.ini")));
    QCOMPARE(paths.desktopSettingsFilePath(),
             QDir(dataDirectory).filePath(QStringLiteral("window.ini")));
    QCOMPARE(paths.diagnosticLogDirectory(),
             QDir(dataDirectory).filePath(QStringLiteral("logs")));
    QCOMPARE(paths.coverCacheDirectory(),
             QDir(dataDirectory).filePath(QStringLiteral("cache/covers")));
    QCOMPARE(paths.customCoverDirectory(),
             QDir(dataDirectory).filePath(QStringLiteral("custom-covers")));
}

void ApplicationPathsTest::enablesPortableModeFromMarker()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile marker(directory.filePath(QStringLiteral("portable.flag")));
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.close();

    const ApplicationPaths paths(directory.path());
    QVERIFY(paths.portableMode());
}

QTEST_GUILESS_MAIN(ApplicationPathsTest)

#include "applicationpathstest.moc"
