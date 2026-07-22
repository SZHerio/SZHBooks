#include "../library/bookcoverprovider.h"
#include "../library/bookmetadata.h"
#include "../library/bookmetadataservice.h"
#include "../library/libraryscanservice.h"

#include <QBuffer>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QtTest>

#include <miniz.h>

namespace {

struct ArchiveEntry final
{
    QByteArray path;
    QByteArray data;
};

bool writeArchive(const QString &filePath, const QList<ArchiveEntry> &entries)
{
    mz_zip_archive archive{};
    if (!mz_zip_writer_init_heap(&archive, 0, 0)) {
        return false;
    }

    bool success = true;
    for (const ArchiveEntry &entry : entries) {
        if (!mz_zip_writer_add_mem(&archive,
                                   entry.path.constData(),
                                   entry.data.constData(),
                                   static_cast<size_t>(entry.data.size()),
                                   MZ_BEST_SPEED)) {
            success = false;
            break;
        }
    }

    void *archiveData = nullptr;
    size_t archiveSize = 0;
    if (success) {
        success = mz_zip_writer_finalize_heap_archive(&archive,
                                                      &archiveData,
                                                      &archiveSize);
    }
    if (success) {
        QFile file(filePath);
        success = file.open(QIODevice::WriteOnly)
                  && file.write(static_cast<const char *>(archiveData),
                                static_cast<qint64>(archiveSize))
                         == static_cast<qint64>(archiveSize);
    }

    mz_free(archiveData);
    mz_zip_writer_end(&archive);
    return success;
}

QByteArray sampleCoverBytes()
{
    QImage cover(120, 180, QImage::Format_RGB32);
    cover.fill(QColor(QStringLiteral("#202020")));

    QByteArray bytes;
    QBuffer buffer(&bytes);
    if (!buffer.open(QIODevice::WriteOnly) || !cover.save(&buffer, "PNG")) {
        return {};
    }
    return bytes;
}

void verifyCoverFile(const BookMetadata &metadata)
{
    QVERIFY(!metadata.fingerprint.isEmpty());
    QVERIFY(metadata.coverUrl.isLocalFile());
    QVERIFY(QFileInfo::exists(metadata.coverUrl.toLocalFile()));
    const QImage cachedCover(metadata.coverUrl.toLocalFile());
    QVERIFY(!cachedCover.isNull());
    QVERIFY(cachedCover.width() > 0);
    QVERIFY(cachedCover.height() > 0);
}

} // namespace

class LibraryServicesTest final : public QObject
{
    Q_OBJECT

private slots:
    void extractsFb2MetadataAndCover();
    void extractsEpubMetadataAndCover();
    void extractsDocxMetadataAndCover();
    void rendersPdfFirstPageCover();
    void scansMetadataInBackground();
    void cancelsBackgroundScan();
};

void LibraryServicesTest::extractsFb2MetadataAndCover()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray coverBytes = sampleCoverBytes();
    QVERIFY(!coverBytes.isEmpty());

    const QString path = directory.filePath(QStringLiteral("covered.fb2"));
    const QString xml = QStringLiteral(R"FB2(<?xml version="1.0" encoding="utf-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0"
             xmlns:l="http://www.w3.org/1999/xlink">
  <description><title-info>
    <book-title>Covered FB2</book-title>
    <author><first-name>Ada</first-name><last-name>Reader</last-name></author>
    <coverpage><image l:href="#cover"/></coverpage>
  </title-info></description>
  <body><section><p>Text</p></section></body>
  <binary id="cover" content-type="image/png">%1</binary>
</FictionBook>)FB2")
                            .arg(QString::fromLatin1(coverBytes.toBase64()));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(xml.toUtf8()), qint64(xml.toUtf8().size()));
    file.close();

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService service(&covers);
    const BookMetadata metadata = service.inspect(QUrl::fromLocalFile(path));
    QCOMPARE(metadata.title, QStringLiteral("Covered FB2"));
    QCOMPARE(metadata.author, QStringLiteral("Ada Reader"));
    QCOMPARE(metadata.formatName, QStringLiteral("FB2"));
    verifyCoverFile(metadata);
    QCOMPARE(covers.cachedCover(metadata.fingerprint), metadata.coverUrl);
}

void LibraryServicesTest::extractsEpubMetadataAndCover()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("covered.epub"));
    const QByteArray coverBytes = sampleCoverBytes();
    QVERIFY(writeArchive(
        path,
        {
            {"META-INF/container.xml",
             R"XML(<?xml version="1.0"?>
<container xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf"/></rootfiles>
</container>)XML"},
            {"OEBPS/content.opf",
             R"OPF(<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf" version="3.0">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/">
    <dc:title>Covered EPUB</dc:title><dc:creator>Octavia Reader</dc:creator>
  </metadata>
  <manifest>
    <item id="cover" href="images/cover.png" media-type="image/png" properties="cover-image"/>
  </manifest>
</package>)OPF"},
            {"OEBPS/images/cover.png", coverBytes}
        }));

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService service(&covers);
    const BookMetadata metadata = service.inspect(QUrl::fromLocalFile(path));
    QCOMPARE(metadata.title, QStringLiteral("Covered EPUB"));
    QCOMPARE(metadata.author, QStringLiteral("Octavia Reader"));
    QCOMPARE(metadata.formatName, QStringLiteral("EPUB"));
    verifyCoverFile(metadata);
}

void LibraryServicesTest::extractsDocxMetadataAndCover()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("covered.docx"));
    QVERIFY(writeArchive(
        path,
        {
            {"docProps/core.xml",
             R"XML(<?xml version="1.0"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
 xmlns:dc="http://purl.org/dc/elements/1.1/">
  <dc:title>Covered DOCX</dc:title><dc:creator>Document Author</dc:creator>
</cp:coreProperties>)XML"},
            {"word/document.xml", R"XML(<w:document xmlns:w="urn:w"><w:body/></w:document>)XML"},
            {"word/media/image1.png", sampleCoverBytes()}
        }));

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService service(&covers);
    const BookMetadata metadata = service.inspect(QUrl::fromLocalFile(path));
    QCOMPARE(metadata.title, QStringLiteral("Covered DOCX"));
    QCOMPARE(metadata.author, QStringLiteral("Document Author"));
    QCOMPARE(metadata.formatName, QStringLiteral("DOCX"));
    verifyCoverFile(metadata);
}

void LibraryServicesTest::rendersPdfFirstPageCover()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("covered.pdf"));
    {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QPageSize::A5));
        writer.setTitle(QStringLiteral("Covered PDF"));
        writer.setAuthor(QStringLiteral("PDF Author"));
        QPainter painter(&writer);
        QVERIFY(painter.isActive());
        painter.fillRect(painter.viewport(), Qt::white);
        painter.setPen(Qt::black);
        painter.drawRect(painter.viewport().adjusted(400, 400, -400, -400));
    }

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService service(&covers);
    const BookMetadata metadata = service.inspect(QUrl::fromLocalFile(path));
    QCOMPARE(metadata.title, QStringLiteral("Covered PDF"));
    QCOMPARE(metadata.author, QStringLiteral("PDF Author"));
    QCOMPARE(metadata.formatName, QStringLiteral("PDF"));
    verifyCoverFile(metadata);
}

void LibraryServicesTest::scansMetadataInBackground()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("background-book.txt"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QVERIFY(file.write("Background scan content") > 0);
    file.close();

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService metadataService(&covers);
    LibraryScanService scanner(&metadataService);
    LibraryBook book;
    book.sourceUrl = QUrl::fromLocalFile(path);
    book.sourcePath = path;
    book.title = QStringLiteral("Cached title");

    QSignalSpy resultSpy(&scanner, &LibraryScanService::resultsReady);
    QSignalSpy finishedSpy(&scanner, &LibraryScanService::finished);
    scanner.start({book});

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.size(), 1, 5000);
    QVERIFY(!finishedSpy.constFirst().constFirst().toBool());
    QCOMPARE(scanner.completed(), 1);
    QCOMPARE(scanner.total(), 1);
    QVERIFY(!resultSpy.isEmpty());

    const QVector<LibraryScanResult> results =
        resultSpy.constFirst().constFirst().value<QVector<LibraryScanResult>>();
    QCOMPARE(results.size(), 1);
    QVERIFY(results.constFirst().fileAvailable);
    QVERIFY(results.constFirst().metadataInspected);
    QCOMPARE(results.constFirst().metadata.title,
             QStringLiteral("background-book"));
    QCOMPARE(results.constFirst().metadata.formatName, QStringLiteral("TXT"));
}

void LibraryServicesTest::cancelsBackgroundScan()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVector<LibraryBook> books;
    for (int index = 0; index < 256; ++index) {
        const QString path = directory.filePath(
            QStringLiteral("book-%1.txt").arg(index));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("Cancelable scan") > 0);
        file.close();

        LibraryBook book;
        book.sourceUrl = QUrl::fromLocalFile(path);
        book.sourcePath = path;
        books.append(book);
    }

    BookCoverProvider covers(directory.filePath(QStringLiteral("covers")));
    BookMetadataService metadataService(&covers);
    LibraryScanService scanner(&metadataService);
    QSignalSpy finishedSpy(&scanner, &LibraryScanService::finished);
    scanner.start(books);
    scanner.cancel();

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.size(), 1, 5000);
    QVERIFY(finishedSpy.constFirst().constFirst().toBool());
    QVERIFY(!scanner.scanning());
    QVERIFY(scanner.completed() <= scanner.total());
}

QTEST_GUILESS_MAIN(LibraryServicesTest)

#include "libraryservicestest.moc"
