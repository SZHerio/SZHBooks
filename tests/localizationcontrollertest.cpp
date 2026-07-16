#include "../localization/localizationcontroller.h"
#include "../storage/localstatestore.h"

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QTemporaryDir>
#include <QtTest>

class LocalizationControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void switchesEmbeddedCatalogAtRuntime();
};

void LocalizationControllerTest::switchesEmbeddedCatalogAtRuntime()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LocalStateStore stateStore(directory.filePath(QStringLiteral("settings.ini")));
    stateStore.setLanguage(QStringLiteral("en"));
    QQmlApplicationEngine engine;
    LocalizationController controller(&stateStore, &engine);

    QCOMPARE(controller.language(), QStringLiteral("en"));
    QCOMPARE(controller.effectiveLanguage(), QStringLiteral("en"));
    QCOMPARE(QCoreApplication::translate("Main", "Library - SZHBooks"),
             QStringLiteral("Library - SZHBooks"));

    controller.setLanguage(QStringLiteral("ru"));
    QCOMPARE(controller.language(), QStringLiteral("ru"));
    QCOMPARE(controller.effectiveLanguage(), QStringLiteral("ru"));
    QCOMPARE(QCoreApplication::translate("Main", "Library - SZHBooks"),
             QStringLiteral("Библиотека - SZHBooks"));

    controller.setLanguage(QStringLiteral("en"));
    QCOMPARE(QCoreApplication::translate("Main", "Library - SZHBooks"),
             QStringLiteral("Library - SZHBooks"));
}

QTEST_MAIN(LocalizationControllerTest)

#include "localizationcontrollertest.moc"
