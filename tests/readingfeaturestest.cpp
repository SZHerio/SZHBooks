#include "../reader/readingsearchcontroller.h"
#include "../storage/readingannotationstore.h"

#include <QTemporaryDir>
#include <QTest>

class ReadingFeaturesTest final : public QObject
{
    Q_OBJECT

private slots:
    void searchFindsAndNavigatesMatches();
    void annotationsPersistPerDocument();
};

void ReadingFeaturesTest::searchFindsAndNavigatesMatches()
{
    ReadingSearchController controller;
    controller.setDocumentText(QStringLiteral("Alpha beta ALPHA alphabet"));
    controller.setQuery(QStringLiteral("alpha"));

    QCOMPARE(controller.resultCount(), 3);
    QCOMPARE(controller.currentIndex(), 0);
    QCOMPARE(controller.currentStart(), 0);
    QCOMPARE(controller.currentLength(), 5);

    controller.next();
    QCOMPARE(controller.currentIndex(), 1);
    QCOMPARE(controller.currentStart(), 11);

    controller.next();
    controller.next();
    QCOMPARE(controller.currentIndex(), 0);

    controller.previous();
    QCOMPARE(controller.currentIndex(), 2);

    controller.setDocumentText(QStringLiteral("No matching text"));
    QCOMPARE(controller.resultCount(), 0);
    QCOMPARE(controller.currentIndex(), -1);
    QCOMPARE(controller.currentStart(), -1);
}

void ReadingFeaturesTest::annotationsPersistPerDocument()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString settingsPath = directory.filePath(QStringLiteral("profile.ini"));
    const QUrl firstDocument = QUrl::fromLocalFile(directory.filePath(QStringLiteral("first.txt")));
    const QUrl secondDocument = QUrl::fromLocalFile(directory.filePath(QStringLiteral("second.txt")));
    QString highlightId;

    {
        ReadingAnnotationStore store(settingsPath);
        store.setDocumentUrl(firstDocument);
        QVERIFY(store.toggleBookmark(0.25, -1, QStringLiteral("Chapter one")));
        highlightId = store.addHighlight(8,
                                         12,
                                         QStringLiteral("  selected\n text  "),
                                         QStringLiteral("Initial note"),
                                         0.3,
                                         -1);
        QVERIFY(!highlightId.isEmpty());
        QCOMPARE(store.totalCount(), 2);
        QCOMPARE(store.bookmarks().size(), 1);
        QCOMPARE(store.highlights().size(), 1);
        store.sync();
    }

    ReadingAnnotationStore restored(settingsPath);
    restored.setDocumentUrl(firstDocument);
    QCOMPARE(restored.totalCount(), 2);
    QVERIFY(restored.hasBookmark(0.251, -1));

    const QVariantMap highlight = restored.highlights().constFirst().toMap();
    QCOMPARE(highlight.value(QStringLiteral("excerpt")).toString(),
             QStringLiteral("selected text"));
    QCOMPARE(highlight.value(QStringLiteral("note")).toString(),
             QStringLiteral("Initial note"));

    restored.updateNote(highlightId, QStringLiteral("Updated note"));
    QCOMPARE(restored.highlights().constFirst().toMap().value(QStringLiteral("note")).toString(),
             QStringLiteral("Updated note"));

    restored.setDocumentUrl(secondDocument);
    QCOMPARE(restored.totalCount(), 0);

    restored.setDocumentUrl(firstDocument);
    restored.removeAnnotation(highlightId);
    QCOMPARE(restored.highlights().size(), 0);
    QCOMPARE(restored.bookmarks().size(), 1);
    QVERIFY(!restored.toggleBookmark(0.25, -1, QString()));
    QCOMPARE(restored.totalCount(), 0);
}

QTEST_MAIN(ReadingFeaturesTest)

#include "readingfeaturestest.moc"
