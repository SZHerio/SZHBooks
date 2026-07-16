#include "../documentreaders.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtCore/private/qzipwriter_p.h>
#include <QtTest>

class DocumentReadersTest final : public QObject
{
    Q_OBJECT

private slots:
    void extractsFb2Chapters();
    void extractsEpubSpineChapters();
};

void DocumentReadersTest::extractsFb2Chapters()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath = directory.filePath(QStringLiteral("sample.fb2"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(R"FB2(<?xml version="1.0" encoding="utf-8"?>
<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">
  <description><title-info><book-title>Sample Book</book-title></title-info></description>
  <body>
    <section><title><p>Chapter One</p></title><p>First chapter text.</p></section>
    <section><title><p>Chapter Two</p></title><p>Second chapter text with more words.</p></section>
  </body>
</FictionBook>)FB2");
    file.close();

    const DocumentLoadResult result = Fb2DocumentReader().load(QFileInfo(filePath));
    QVERIFY(result.isSuccess());
    QCOMPARE(result.title, QStringLiteral("Sample Book"));
    QCOMPARE(result.chapters.size(), 2);
    QCOMPARE(result.chapters.at(0).title, QStringLiteral("Chapter One"));
    QCOMPARE(result.chapters.at(0).progress, 0.0);
    QCOMPARE(result.chapters.at(1).title, QStringLiteral("Chapter Two"));
    QVERIFY(result.chapters.at(1).progress > result.chapters.at(0).progress);
}

void DocumentReadersTest::extractsEpubSpineChapters()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString filePath = directory.filePath(QStringLiteral("sample.epub"));
    QZipWriter zip(filePath);
    QVERIFY(zip.isWritable());
    zip.addFile(QStringLiteral("META-INF/container.xml"),
                R"XML(<?xml version="1.0"?>
<container xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles><rootfile full-path="OEBPS/content.opf"/></rootfiles>
</container>)XML");
    zip.addFile(QStringLiteral("OEBPS/content.opf"),
                R"OPF(<?xml version="1.0"?>
<package xmlns="http://www.idpf.org/2007/opf">
  <metadata xmlns:dc="http://purl.org/dc/elements/1.1/"><dc:title>EPUB Sample</dc:title></metadata>
  <manifest>
    <item id="one" href="one.xhtml" media-type="application/xhtml+xml"/>
    <item id="two" href="two.xhtml" media-type="application/xhtml+xml"/>
  </manifest>
  <spine><itemref idref="one"/><itemref idref="two"/></spine>
</package>)OPF");
    zip.addFile(QStringLiteral("OEBPS/one.xhtml"),
                R"HTML(<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Fallback One</title></head>
<body><h1>Opening</h1><p>Opening chapter text.</p></body></html>)HTML");
    zip.addFile(QStringLiteral("OEBPS/two.xhtml"),
                R"HTML(<html xmlns="http://www.w3.org/1999/xhtml"><head><title>Fallback Two</title></head>
<body><h2>Arrival</h2><p>Arrival chapter text with additional words.</p></body></html>)HTML");
    zip.close();
    QCOMPARE(zip.status(), QZipWriter::NoError);

    const DocumentLoadResult result = EpubDocumentReader().load(QFileInfo(filePath));
    QVERIFY(result.isSuccess());
    QCOMPARE(result.title, QStringLiteral("EPUB Sample"));
    QCOMPARE(result.chapters.size(), 2);
    QCOMPARE(result.chapters.at(0).title, QStringLiteral("Opening"));
    QCOMPARE(result.chapters.at(0).progress, 0.0);
    QCOMPARE(result.chapters.at(1).title, QStringLiteral("Arrival"));
    QVERIFY(result.chapters.at(1).progress > result.chapters.at(0).progress);
    QVERIFY(result.text.contains(QStringLiteral("Opening chapter text.")));
    QVERIFY(result.text.contains(QStringLiteral("Arrival chapter text")));
}

QTEST_GUILESS_MAIN(DocumentReadersTest)

#include "documentreaderstest.moc"
