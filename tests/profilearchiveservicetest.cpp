#include "../archive/ziparchivereader.h"
#include "../archive/ziparchivewriter.h"
#include "../storage/localstatestore.h"
#include "../storage/profilearchiveservice.h"
#include "../storage/readingannotationstore.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

class ProfileArchiveServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void exportsAndRestoresCompleteLocalProfile();
    void rejectsDamagedBackupWithoutReplacingProfile();
    void rejectsForeignArchive();
};

void ProfileArchiveServiceTest::exportsAndRestoresCompleteLocalProfile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString settingsPath = directory.filePath(QStringLiteral("settings.ini"));
    const QString bookPath = directory.filePath(QStringLiteral("book.txt"));
    QFile bookFile(bookPath);
    QVERIFY(bookFile.open(QIODevice::WriteOnly));
    QCOMPARE(bookFile.write("Profile archive fixture"), qint64(23));
    bookFile.close();

    const QUrl bookUrl = QUrl::fromLocalFile(bookPath);
    LocalStateStore store(settingsPath);
    ReadingAnnotationStore annotations(settingsPath);
    store.setColorTheme(QStringLiteral("dark"));
    store.setReadingFont(QStringLiteral("mono"));
    store.setTextFontSize(24);
    store.setLineHeight(1.75);
    store.setLibrarySortMode(QStringLiteral("author"));
    store.setLibraryViewMode(QStringLiteral("list"));
    store.recordBookOpened(bookUrl,
                           QStringLiteral("Portable title"),
                           QStringLiteral("Portable author"),
                           QStringLiteral("TXT"));
    store.saveTextPosition(bookUrl, 0.42);

    annotations.setDocumentUrl(bookUrl);
    QVERIFY(annotations.toggleBookmark(0.42, -1, QStringLiteral("Chapter")));
    QVERIFY(!annotations.addHighlight(4,
                                      7,
                                      QStringLiteral("archive"),
                                      QStringLiteral("Remember this"),
                                      0.42,
                                      -1)
                 .isEmpty());
    annotations.sync();
    store.sync();

    ProfileArchiveService service(&store);
    QSignalSpy exportedSpy(&service, &ProfileArchiveService::profileExported);
    const QString requestedPath = directory.filePath(QStringLiteral("reader-profile"));
    QVERIFY(service.exportProfile(QUrl::fromLocalFile(requestedPath)));
    QCOMPARE(exportedSpy.count(), 1);

    const QString backupPath = requestedPath + QStringLiteral(".szhbackup");
    QVERIFY(QFileInfo::exists(backupPath));
    const ZipArchiveReader archive(backupPath);
    QVERIFY2(archive.isOpen(), qPrintable(archive.errorString()));
    QCOMPARE(archive.filePaths(),
             QStringList({QStringLiteral("manifest.json"),
                          QStringLiteral("profile.json")}));

    QString replaceError;
    QVERIFY(store.replaceProfileValues({}, &replaceError));
    annotations.reload();
    QCOMPARE(store.colorTheme(), QStringLiteral("light"));
    QCOMPARE(store.libraryBooks().size(), 0);
    QCOMPARE(annotations.totalCount(), 0);

    QSignalSpy importedSpy(&service, &ProfileArchiveService::profileImported);
    QVERIFY2(service.importProfile(QUrl::fromLocalFile(backupPath)),
             qPrintable(service.errorMessage()));
    QCOMPARE(importedSpy.count(), 1);
    annotations.reload();

    QCOMPARE(store.colorTheme(), QStringLiteral("dark"));
    QCOMPARE(store.readingFont(), QStringLiteral("mono"));
    QCOMPARE(store.textFontSize(), 24);
    QCOMPARE(store.lineHeight(), 1.75);
    QCOMPARE(store.librarySortMode(), QStringLiteral("author"));
    QCOMPARE(store.libraryViewMode(), QStringLiteral("list"));
    QCOMPARE(store.textPosition(bookUrl), 0.42);
    QCOMPARE(store.libraryBooks().size(), 1);
    QCOMPARE(store.libraryBooks().constFirst().title,
             QStringLiteral("Portable title"));
    QCOMPARE(annotations.totalCount(), 2);
    QCOMPARE(annotations.bookmarks().size(), 1);
    QCOMPARE(annotations.highlights().size(), 1);
}

void ProfileArchiveServiceTest::rejectsDamagedBackupWithoutReplacingProfile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LocalStateStore store(directory.filePath(QStringLiteral("settings.ini")));
    store.setTextFontSize(19);
    ProfileArchiveService service(&store);
    const QString backupPath = directory.filePath(QStringLiteral("valid.szhbackup"));
    QVERIFY(service.exportProfile(QUrl::fromLocalFile(backupPath)));

    store.setTextFontSize(31);
    QFile backup(backupPath);
    QVERIFY(backup.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(backup.write("not a zip archive") > 0);
    backup.close();

    QSignalSpy failedSpy(&service, &ProfileArchiveService::operationFailed);
    QVERIFY(!service.importProfile(QUrl::fromLocalFile(backupPath)));
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(!service.errorMessage().isEmpty());
    QCOMPARE(store.textFontSize(), 31);
}

void ProfileArchiveServiceTest::rejectsForeignArchive()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString archivePath = directory.filePath(QStringLiteral("foreign.szhbackup"));
    ZipArchiveWriter archive;
    QVERIFY(archive.addFile(QStringLiteral("readme.txt"), QByteArray("foreign")));
    QVERIFY2(archive.writeTo(archivePath), qPrintable(archive.errorString()));

    LocalStateStore store(directory.filePath(QStringLiteral("settings.ini")));
    store.setColorTheme(QStringLiteral("dark"));
    ProfileArchiveService service(&store);
    QVERIFY(!service.importProfile(QUrl::fromLocalFile(archivePath)));
    QCOMPARE(store.colorTheme(), QStringLiteral("dark"));
}

QTEST_GUILESS_MAIN(ProfileArchiveServiceTest)

#include "profilearchiveservicetest.moc"
