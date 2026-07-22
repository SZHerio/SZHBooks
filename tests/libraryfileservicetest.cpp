#include "../library/bookcoverprovider.h"
#include "../library/bookmetadataservice.h"
#include "../library/libraryfileservice.h"
#include "../library/libraryrepository.h"
#include "../storage/localstatestore.h"
#include "../storage/readingannotationstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

namespace {

bool writeFile(const QString &path, const QByteArray &data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly)
           && file.write(data) == data.size();
}

struct LibraryFixture final
{
    explicit LibraryFixture(const QString &directoryPath)
        : rootPath(QDir(directoryPath).filePath(QStringLiteral("OneDrive/SZHBooks")))
        , store(QDir(directoryPath).filePath(QStringLiteral("profile.ini")))
        , coverProvider(QDir(directoryPath).filePath(QStringLiteral("covers")))
        , metadataService(&coverProvider)
        , repository(&store, &metadataService)
        , files(&repository)
    {
        QDir().mkpath(rootPath);
        files.setRootPath(rootPath);
    }

    QString rootPath;
    LocalStateStore store;
    BookCoverProvider coverProvider;
    BookMetadataService metadataService;
    LibraryRepository repository;
    LibraryFileService files;
};

} // namespace

class LibraryFileServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void importsBatchAndSkipsContentDuplicates();
    void cancelsBatchAfterCurrentFile();
    void preservesReadingStateAcrossFileAndFolderMoves();
    void validatesManagedBoundariesAndEmptyFolderRemoval();
};

void LibraryFileServiceTest::importsBatchAndSkipsContentDuplicates()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    LibraryFixture fixture(directory.path());
    QVERIFY(fixture.files.createCollection({}, QStringLiteral("Fiction")));

    const QString firstPath = directory.filePath(QStringLiteral("first.txt"));
    const QString duplicatePath = directory.filePath(QStringLiteral("duplicate.txt"));
    QVERIFY(writeFile(firstPath, "The same book"));
    QVERIFY(writeFile(duplicatePath, "The same book"));

    QSignalSpy finishedSpy(&fixture.files, &LibraryFileService::batchFinished);
    const QVariantList sources{
        QUrl::fromLocalFile(firstPath),
        QUrl::fromLocalFile(duplicatePath)
    };
    QVERIFY(fixture.files.importBooks(sources, QStringLiteral("Fiction")));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 10000);

    const QList<QVariant> result = finishedSpy.takeFirst();
    QCOMPARE(result.at(0).toInt(), 1);
    QCOMPARE(result.at(1).toInt(), 1);
    QCOMPARE(result.at(2).toInt(), 0);
    QCOMPARE(result.at(3).toBool(), false);
    QCOMPARE(fixture.repository.books().size(), 1);
    const QUrl managedUrl = fixture.files.lastImportedUrl();
    QVERIFY(managedUrl.isLocalFile());
    QCOMPARE(fixture.files.collectionForBook(managedUrl),
             QStringLiteral("Fiction"));

    QVERIFY(fixture.files.importBooks({managedUrl}, QStringLiteral("Fiction")));
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 10000);
    const QList<QVariant> managedResult = finishedSpy.takeFirst();
    QCOMPARE(managedResult.at(0).toInt(), 0);
    QCOMPARE(managedResult.at(1).toInt(), 1);
    QCOMPARE(managedResult.at(2).toInt(), 0);
}

void LibraryFileServiceTest::cancelsBatchAfterCurrentFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    LibraryFixture fixture(directory.path());

    const QString firstPath = directory.filePath(QStringLiteral("first.txt"));
    const QString secondPath = directory.filePath(QStringLiteral("second.txt"));
    QVERIFY(writeFile(firstPath, "First book"));
    QVERIFY(writeFile(secondPath, "Second book"));

    QSignalSpy finishedSpy(&fixture.files, &LibraryFileService::batchFinished);
    QVERIFY(fixture.files.importBooks({QUrl::fromLocalFile(firstPath),
                                       QUrl::fromLocalFile(secondPath)}));
    QVERIFY(!fixture.files.createCollection({}, QStringLiteral("Blocked")));
    QVERIFY(!fixture.files.errorMessage().isEmpty());
    fixture.files.cancel();
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 10000);

    const QList<QVariant> result = finishedSpy.takeFirst();
    QCOMPARE(result.at(3).toBool(), true);
    QCOMPARE(fixture.files.operationTotal(), 2);
    QCOMPARE(fixture.files.operationCompleted(), 1);
    QCOMPARE(fixture.repository.books().size(), 1);
    QVERIFY(!fixture.files.busy());
}

void LibraryFileServiceTest::preservesReadingStateAcrossFileAndFolderMoves()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    LibraryFixture fixture(directory.path());
    QVERIFY(fixture.files.createCollection({}, QStringLiteral("Shelf")));
    QVERIFY(fixture.files.createCollection(QStringLiteral("Shelf"),
                                           QStringLiteral("Classics")));

    const QString sourcePath = directory.filePath(QStringLiteral("novel.txt"));
    QVERIFY(writeFile(sourcePath, "A novel"));
    QUrl bookUrl = fixture.files.importBook(QUrl::fromLocalFile(sourcePath),
                                            QStringLiteral("Shelf/Classics"));
    QVERIFY(!bookUrl.isEmpty());
    fixture.store.saveTextPosition(bookUrl, 0.37);
    ReadingAnnotationStore annotations(fixture.store.settingsFilePath());
    annotations.setDocumentUrl(bookUrl);
    QVERIFY(annotations.toggleBookmark(0.37, -1, QStringLiteral("Saved place")));
    annotations.sync();

    bookUrl = fixture.files.moveBook(bookUrl, QStringLiteral("Shelf"));
    QVERIFY(!bookUrl.isEmpty());
    QVERIFY(qAbs(fixture.store.textPosition(bookUrl) - 0.37) < 0.0001);
    annotations.reload();
    annotations.setDocumentUrl(bookUrl);
    QCOMPARE(annotations.bookmarks().size(), 1);

    bookUrl = fixture.files.renameBook(bookUrl, QStringLiteral("Renamed novel"));
    QVERIFY(!bookUrl.isEmpty());
    QCOMPARE(QFileInfo(bookUrl.toLocalFile()).fileName(),
             QStringLiteral("Renamed novel.txt"));
    QVERIFY(qAbs(fixture.store.textPosition(bookUrl) - 0.37) < 0.0001);

    const QString renamedCollection = fixture.files.renameCollection(
        QStringLiteral("Shelf"), QStringLiteral("Fiction"));
    QCOMPARE(renamedCollection, QStringLiteral("Fiction"));
    bookUrl = fixture.repository.books().constFirst().sourceUrl;
    QCOMPARE(fixture.files.collectionForBook(bookUrl), QStringLiteral("Fiction"));
    QVERIFY(qAbs(fixture.store.textPosition(bookUrl) - 0.37) < 0.0001);

    bookUrl = fixture.files.moveBook(bookUrl, {});
    QVERIFY(!bookUrl.isEmpty());
    QVERIFY(fixture.files.removeCollection(QStringLiteral("Fiction/Classics")));
    QVERIFY(fixture.files.removeCollection(QStringLiteral("Fiction")));

    const QString managedPath = bookUrl.toLocalFile();
    QVERIFY(fixture.files.removeBook(bookUrl, false));
    QVERIFY(QFileInfo::exists(managedPath));
    QCOMPARE(fixture.repository.books().size(), 0);
    QCOMPARE(fixture.files.importBook(bookUrl), bookUrl);
    QCOMPARE(fixture.repository.books().size(), 1);
}

void LibraryFileServiceTest::validatesManagedBoundariesAndEmptyFolderRemoval()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    LibraryFixture fixture(directory.path());

    QVERIFY(!fixture.files.createCollection({}, QStringLiteral("..")));
    QVERIFY(!fixture.files.createCollection(QStringLiteral("../Outside"),
                                            QStringLiteral("Unexpected")));
    QVERIFY(!QFileInfo::exists(QDir(fixture.rootPath).filePath(
        QStringLiteral("Unexpected"))));
    QVERIFY(fixture.files.renameCollection({}, QStringLiteral("Renamed")).isEmpty());
    QVERIFY(!fixture.files.removeCollection({}));

    QVERIFY(fixture.files.createCollection({}, QStringLiteral("Temporary")));
    const QString nestedFile = QDir(fixture.rootPath).filePath(
        QStringLiteral("Temporary/note.txt"));
    QVERIFY(writeFile(nestedFile, "not empty"));
    QVERIFY(!fixture.files.removeCollection(QStringLiteral("Temporary")));
    QVERIFY(QFile::remove(nestedFile));
    QVERIFY(fixture.files.removeCollection(QStringLiteral("Temporary")));

    const QString outsidePath = directory.filePath(QStringLiteral("outside.txt"));
    QVERIFY(writeFile(outsidePath, "outside"));
    const QUrl outsideUrl = QUrl::fromLocalFile(outsidePath);
    QVERIFY(!fixture.files.isManagedBook(outsideUrl));
    QVERIFY(fixture.files.importBook(outsideUrl,
                                     QStringLiteral("../Outside")).isEmpty());
    QVERIFY(!QFileInfo::exists(QDir(fixture.rootPath).filePath(
        QStringLiteral("outside.txt"))));
    QVERIFY(fixture.files.moveBook(outsideUrl, {}).isEmpty());
    QVERIFY(!fixture.files.removeBook(outsideUrl, true));
    QVERIFY(!fixture.files.removeBook({}, false));
    QVERIFY(QFileInfo::exists(outsidePath));
}

QTEST_GUILESS_MAIN(LibraryFileServiceTest)

#include "libraryfileservicetest.moc"
