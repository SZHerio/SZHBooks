#include "../search/librarysearchindex.h"

#include <QFile>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QtTest>

namespace {

bool writeUtf8File(const QString &filePath, const QString &text)
{
    QFile file(filePath);
    return file.open(QIODevice::WriteOnly)
           && file.write(text.toUtf8()) == text.toUtf8().size();
}

LibraryBook localBook(const QString &filePath,
                      const QString &title,
                      const QString &formatName)
{
    LibraryBook book;
    book.sourcePath = filePath;
    book.sourceUrl = QUrl::fromLocalFile(filePath);
    book.title = title;
    book.author = QStringLiteral("Test Author");
    book.formatName = formatName;
    book.fileAvailable = true;
    return book;
}

bool writePdf(const QString &filePath)
{
    QPdfWriter writer(filePath);
    writer.setTitle(QStringLiteral("PDF Search Test"));
    QPainter painter(&writer);
    if (!painter.isActive()) {
        return false;
    }
    painter.drawText(QRect(300, 300, 5000, 1000),
                     Qt::AlignLeft,
                     QStringLiteral("First page without the target"));
    writer.newPage();
    painter.drawText(QRect(300, 300, 5000, 1000),
                     Qt::AlignLeft,
                     QStringLiteral("NebulaQuartz appears on page two"));
    painter.end();
    return true;
}

} // namespace

class LibrarySearchIndexTest final : public QObject
{
    Q_OBJECT

private slots:
    void indexesTextAndPreservesCharacterLocation();
    void indexesPdfPagesAndRemovesStaleBooks();
    void reportsProgressAndStopsBetweenBooks();
};

void LibrarySearchIndexTest::indexesTextAndPreservesCharacterLocation()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString textPath = directory.filePath(QStringLiteral("long-book.txt"));
    const QString uniquePhrase = QStringLiteral("SilverCometArchive");
    const QString text = QStringLiteral("opening paragraph ").repeated(190)
                         + QStringLiteral("\n\n") + uniquePhrase
                         + QStringLiteral(" closes this chapter");
    QVERIFY(writeUtf8File(textPath, text));

    const QString indexPath = directory.filePath(QStringLiteral("search.sqlite"));
    LibrarySearchIndex index(indexPath);
    const LibraryBook book = localBook(textPath, QStringLiteral("Long Book"),
                                       QStringLiteral("TXT"));
    const LibrarySearchIndexSummary summary = index.synchronize({book});
    QVERIFY2(summary.errorMessage.isEmpty(), qPrintable(summary.errorMessage));
    QCOMPARE(summary.totalBooks, 1);
    QCOMPARE(summary.indexedBooks, 1);
    QCOMPARE(summary.failedBooks, 0);

    QString searchError;
    const QVector<LibrarySearchHit> hits = index.search(QStringLiteral("SilverComet"),
                                                        20,
                                                        &searchError);
    QVERIFY2(searchError.isEmpty(), qPrintable(searchError));
    QCOMPARE(hits.size(), 1);
    QCOMPARE(hits.constFirst().sourceUrl, book.sourceUrl);
    QVERIFY(hits.constFirst().start > 0);
    QVERIFY(hits.constFirst().start <= text.indexOf(uniquePhrase));
    QVERIFY(hits.constFirst().progress > 0);
    QVERIFY(hits.constFirst().snippet.contains(uniquePhrase, Qt::CaseInsensitive));

    const QVector<LibrarySearchHit> metadataHits =
        index.search(QStringLiteral("Long Book"), 20, &searchError);
    QCOMPARE(metadataHits.size(), 1);
    QCOMPARE(metadataHits.constFirst().bookTitle, QStringLiteral("Long Book"));

    const QString replacementText = QStringLiteral(
        "The revised edition contains the unique signal AuroraLedger.");
    QVERIFY(writeUtf8File(textPath, replacementText));
    const LibrarySearchIndexSummary updatedSummary = index.synchronize({book});
    QVERIFY2(updatedSummary.errorMessage.isEmpty(),
             qPrintable(updatedSummary.errorMessage));
    QVERIFY(index.search(QStringLiteral("SilverComet"), 20, &searchError).isEmpty());
    QCOMPARE(index.search(QStringLiteral("AuroraLedger"), 20, &searchError).size(), 1);
}

void LibrarySearchIndexTest::indexesPdfPagesAndRemovesStaleBooks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString pdfPath = directory.filePath(QStringLiteral("pages.pdf"));
    QVERIFY(writePdf(pdfPath));

    LibrarySearchIndex index(directory.filePath(QStringLiteral("search.sqlite")));
    const LibraryBook book = localBook(pdfPath, QStringLiteral("Pages"),
                                       QStringLiteral("PDF"));
    const LibrarySearchIndexSummary summary = index.synchronize({book});
    QVERIFY2(summary.errorMessage.isEmpty(), qPrintable(summary.errorMessage));
    QCOMPARE(summary.indexedBooks, 1);

    QString searchError;
    const QVector<LibrarySearchHit> hits = index.search(QStringLiteral("NebulaQuartz"),
                                                        20,
                                                        &searchError);
    QVERIFY2(searchError.isEmpty(), qPrintable(searchError));
    QCOMPARE(hits.size(), 1);
    QCOMPARE(hits.constFirst().page, 1);
    QCOMPARE(hits.constFirst().start, -1);

    const LibrarySearchIndexSummary emptySummary = index.synchronize({});
    QVERIFY2(emptySummary.errorMessage.isEmpty(), qPrintable(emptySummary.errorMessage));
    QCOMPARE(emptySummary.indexedBooks, 0);
    QVERIFY(index.search(QStringLiteral("NebulaQuartz"), 20, &searchError).isEmpty());
}

void LibrarySearchIndexTest::reportsProgressAndStopsBetweenBooks()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVector<LibraryBook> books;
    for (int index = 0; index < 3; ++index) {
        const QString path = directory.filePath(QStringLiteral("book-%1.txt").arg(index));
        QVERIFY(writeUtf8File(path,
                              QStringLiteral("Searchable content %1").arg(index)));
        books.append(localBook(path,
                               QStringLiteral("Book %1").arg(index),
                               QStringLiteral("TXT")));
    }

    std::atomic_bool cancellation = false;
    QVector<int> progressValues;
    const LibrarySearchIndexSummary summary = LibrarySearchIndex(
        directory.filePath(QStringLiteral("cancel-search.sqlite")))
        .synchronize(books,
                     false,
                     &cancellation,
                     [&cancellation, &progressValues](int completed, int) {
                         progressValues.append(completed);
                         if (completed == 1) {
                             cancellation.store(true, std::memory_order_relaxed);
                         }
                     });

    QVERIFY(summary.canceled);
    QCOMPARE(summary.totalBooks, 3);
    QCOMPARE(summary.processedBooks, 1);
    QCOMPARE(progressValues.constFirst(), 0);
    QCOMPARE(progressValues.constLast(), 1);
}

QTEST_MAIN(LibrarySearchIndexTest)

#include "librarysearchindextest.moc"
