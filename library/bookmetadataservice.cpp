#include "bookmetadataservice.h"

#include "bookcoverprovider.h"
#include "../archive/ziparchivereader.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QImageReader>
#include <QPdfDocument>
#include <QSet>
#include <QUrl>

namespace {

constexpr int maximumDecodedCoverPixels = 40 * 1000 * 1000;
constexpr int maximumDocxImagesToInspect = 24;

struct ExtractedBookData final
{
    QString title;
    QString author;
    QImage cover;
};

struct EpubManifestItem final
{
    QString id;
    QString href;
    QString mediaType;
    QString properties;
};

QString elementName(const QDomElement &element)
{
    QString name = element.localName();
    if (name.isEmpty()) {
        name = element.tagName();
    }
    const qsizetype separator = name.indexOf(u':');
    return separator >= 0 ? name.mid(separator + 1) : name;
}

QDomElement firstElementByName(const QDomNode &root, const QString &name)
{
    for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
        const QDomElement element = node.toElement();
        if (!element.isNull() && elementName(element) == name) {
            return element;
        }
        const QDomElement nested = firstElementByName(node, name);
        if (!nested.isNull()) {
            return nested;
        }
    }
    return {};
}

QDomElement directChildElementByName(const QDomNode &root, const QString &name)
{
    for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
        const QDomElement element = node.toElement();
        if (!element.isNull() && elementName(element) == name) {
            return element;
        }
    }
    return {};
}

QList<QDomElement> elementsByName(const QDomNode &root, const QString &name)
{
    QList<QDomElement> elements;
    for (QDomNode node = root.firstChild(); !node.isNull(); node = node.nextSibling()) {
        const QDomElement element = node.toElement();
        if (!element.isNull() && elementName(element) == name) {
            elements.append(element);
        }
        elements.append(elementsByName(node, name));
    }
    return elements;
}

QString attributeByName(const QDomElement &element, const QString &name)
{
    const QDomNamedNodeMap attributes = element.attributes();
    for (int index = 0; index < attributes.count(); ++index) {
        const QDomAttr attribute = attributes.item(index).toAttr();
        QString attributeName = attribute.localName();
        if (attributeName.isEmpty()) {
            attributeName = attribute.name();
        }
        const qsizetype separator = attributeName.indexOf(u':');
        if (separator >= 0) {
            attributeName = attributeName.mid(separator + 1);
        }
        if (attributeName == name) {
            return attribute.value();
        }
    }
    return {};
}

QString normalizedText(QString text)
{
    text.replace(QChar::Nbsp, u' ');
    return text.simplified();
}

QString joinedValues(const QDomNode &root, const QString &name)
{
    QStringList values;
    for (const QDomElement &element : elementsByName(root, name)) {
        const QString value = normalizedText(element.text());
        if (!value.isEmpty() && !values.contains(value)) {
            values.append(value);
        }
    }
    return values.join(QStringLiteral(", "));
}

QString fb2Author(const QDomElement &titleInfo)
{
    QStringList authors;
    for (const QDomElement &authorElement : elementsByName(titleInfo, QStringLiteral("author"))) {
        QStringList parts;
        for (const QString &partName : {QStringLiteral("first-name"),
                                        QStringLiteral("middle-name"),
                                        QStringLiteral("last-name")}) {
            const QString part = normalizedText(
                directChildElementByName(authorElement, partName).text());
            if (!part.isEmpty()) {
                parts.append(part);
            }
        }
        QString author = parts.join(u' ');
        if (author.isEmpty()) {
            author = normalizedText(
                directChildElementByName(authorElement, QStringLiteral("nickname")).text());
        }
        if (!author.isEmpty() && !authors.contains(author)) {
            authors.append(author);
        }
    }
    return authors.join(QStringLiteral(", "));
}

QImage decodedImage(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        return {};
    }

    QBuffer buffer;
    buffer.setData(bytes);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }

    QImageReader reader(&buffer);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (size.isValid()
        && static_cast<qint64>(size.width()) * size.height() > maximumDecodedCoverPixels) {
        return {};
    }
    return reader.read();
}

QString cleanArchivePath(const QString &path)
{
    const QString normalized = QString(path).replace(u'\\', u'/');
    QStringList parts;
    for (const QString &part : normalized.split(u'/', Qt::SkipEmptyParts)) {
        if (part == QLatin1String(".")) {
            continue;
        }
        if (part == QLatin1String("..")) {
            if (!parts.isEmpty()) {
                parts.removeLast();
            }
            continue;
        }
        parts.append(part);
    }
    return parts.join(u'/');
}

QString archiveDirectory(const QString &path)
{
    const qsizetype separator = path.lastIndexOf(u'/');
    return separator < 0 ? QString() : path.left(separator);
}

QString resolveArchivePath(const QString &baseDirectory, QString href)
{
    const qsizetype fragment = href.indexOf(u'#');
    if (fragment >= 0) {
        href.truncate(fragment);
    }
    const qsizetype query = href.indexOf(u'?');
    if (query >= 0) {
        href.truncate(query);
    }
    href = QUrl::fromPercentEncoding(href.toUtf8());
    if (href.startsWith(u'/')) {
        return cleanArchivePath(href.mid(1));
    }
    return cleanArchivePath(baseDirectory.isEmpty() ? href : baseDirectory + u'/' + href);
}

QByteArray archiveData(const ZipArchiveReader &archive, const QString &path)
{
    QByteArray data = archive.fileData(path);
    if (!data.isEmpty()) {
        return data;
    }

    const QString decodedPath = cleanArchivePath(QUrl::fromPercentEncoding(path.toUtf8()));
    data = archive.fileData(decodedPath);
    if (!data.isEmpty()) {
        return data;
    }

    for (const QString &candidate : archive.filePaths()) {
        if (candidate.compare(decodedPath, Qt::CaseInsensitive) == 0) {
            return archive.fileData(candidate);
        }
    }
    return {};
}

QString firstArchivePathWithSuffix(const ZipArchiveReader &archive, const QString &suffix)
{
    for (const QString &path : archive.filePaths()) {
        if (path.endsWith(suffix, Qt::CaseInsensitive)) {
            return path;
        }
    }
    return {};
}

bool containsToken(const QString &values, const QString &token)
{
    return values.simplified().split(u' ', Qt::SkipEmptyParts).contains(token);
}

ExtractedBookData extractFb2(const QFileInfo &fileInfo)
{
    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QDomDocument document;
    if (!document.setContent(file.readAll())) {
        return {};
    }

    ExtractedBookData data;
    const QDomElement description = firstElementByName(document, QStringLiteral("description"));
    const QDomElement titleInfo = firstElementByName(description, QStringLiteral("title-info"));
    data.title = normalizedText(
        directChildElementByName(titleInfo, QStringLiteral("book-title")).text());
    data.author = fb2Author(titleInfo);

    const QDomElement coverPage = firstElementByName(titleInfo, QStringLiteral("coverpage"));
    QString coverId = attributeByName(
        firstElementByName(coverPage, QStringLiteral("image")), QStringLiteral("href"));
    if (coverId.startsWith(u'#')) {
        coverId.remove(0, 1);
    }
    if (!coverId.isEmpty()) {
        for (const QDomElement &binary : elementsByName(document, QStringLiteral("binary"))) {
            if (attributeByName(binary, QStringLiteral("id")) == coverId) {
                data.cover = decodedImage(
                    QByteArray::fromBase64(binary.text().toLatin1()));
                break;
            }
        }
    }
    return data;
}

ExtractedBookData extractEpub(const QFileInfo &fileInfo)
{
    const ZipArchiveReader archive(fileInfo.absoluteFilePath());
    if (!archive.isOpen()) {
        return {};
    }

    QString packagePath;
    QDomDocument container;
    if (container.setContent(archiveData(archive, QStringLiteral("META-INF/container.xml")))) {
        packagePath = cleanArchivePath(QUrl::fromPercentEncoding(
            attributeByName(firstElementByName(container, QStringLiteral("rootfile")),
                            QStringLiteral("full-path"))
                .toUtf8()));
    }
    if (packagePath.isEmpty()) {
        packagePath = firstArchivePathWithSuffix(archive, QStringLiteral(".opf"));
    }

    QDomDocument package;
    if (packagePath.isEmpty()
        || !package.setContent(archiveData(archive, packagePath))) {
        return {};
    }

    ExtractedBookData data;
    const QDomElement metadata = firstElementByName(package, QStringLiteral("metadata"));
    data.title = normalizedText(firstElementByName(metadata, QStringLiteral("title")).text());
    data.author = joinedValues(metadata, QStringLiteral("creator"));

    QHash<QString, EpubManifestItem> manifest;
    for (const QDomElement &item : elementsByName(package, QStringLiteral("item"))) {
        EpubManifestItem manifestItem;
        manifestItem.id = attributeByName(item, QStringLiteral("id"));
        manifestItem.href = attributeByName(item, QStringLiteral("href"));
        manifestItem.mediaType = attributeByName(item, QStringLiteral("media-type"));
        manifestItem.properties = attributeByName(item, QStringLiteral("properties"));
        manifest.insert(manifestItem.id, manifestItem);
    }

    QString legacyCoverId;
    for (const QDomElement &meta : elementsByName(metadata, QStringLiteral("meta"))) {
        if (attributeByName(meta, QStringLiteral("name")).compare(
                QStringLiteral("cover"), Qt::CaseInsensitive) == 0) {
            legacyCoverId = attributeByName(meta, QStringLiteral("content"));
            break;
        }
    }

    EpubManifestItem coverItem = manifest.value(legacyCoverId);
    if (coverItem.href.isEmpty()) {
        for (const EpubManifestItem &item : manifest) {
            if (containsToken(item.properties, QStringLiteral("cover-image"))) {
                coverItem = item;
                break;
            }
        }
    }
    if (coverItem.href.isEmpty()) {
        for (const EpubManifestItem &item : manifest) {
            if (item.mediaType.startsWith(QStringLiteral("image/"))
                && (item.id.contains(QStringLiteral("cover"), Qt::CaseInsensitive)
                    || item.href.contains(QStringLiteral("cover"), Qt::CaseInsensitive))) {
                coverItem = item;
                break;
            }
        }
    }

    if (!coverItem.href.isEmpty()) {
        const QString coverPath = resolveArchivePath(archiveDirectory(packagePath),
                                                     coverItem.href);
        data.cover = decodedImage(archiveData(archive, coverPath));
    }
    return data;
}

ExtractedBookData extractDocx(const QFileInfo &fileInfo)
{
    const ZipArchiveReader archive(fileInfo.absoluteFilePath());
    if (!archive.isOpen()) {
        return {};
    }

    ExtractedBookData data;
    QDomDocument properties;
    if (properties.setContent(archiveData(archive, QStringLiteral("docProps/core.xml")))) {
        data.title = normalizedText(
            firstElementByName(properties, QStringLiteral("title")).text());
        data.author = normalizedText(
            firstElementByName(properties, QStringLiteral("creator")).text());
    }

    qint64 bestArea = 0;
    int inspectedImages = 0;
    for (const QString &path : archive.filePaths()) {
        if (!path.startsWith(QStringLiteral("word/media/"), Qt::CaseInsensitive)) {
            continue;
        }
        if (++inspectedImages > maximumDocxImagesToInspect) {
            break;
        }

        const QImage candidate = decodedImage(archiveData(archive, path));
        const qint64 area = static_cast<qint64>(candidate.width()) * candidate.height();
        if (!candidate.isNull() && area > bestArea) {
            bestArea = area;
            data.cover = candidate;
        }
    }
    return data;
}

ExtractedBookData extractPdf(const QFileInfo &fileInfo)
{
    QPdfDocument document;
    if (document.load(fileInfo.absoluteFilePath()) != QPdfDocument::Error::None) {
        return {};
    }

    ExtractedBookData data;
    data.title = document.metaData(QPdfDocument::MetaDataField::Title).toString().trimmed();
    data.author = document.metaData(QPdfDocument::MetaDataField::Author).toString().trimmed();
    if (document.pageCount() > 0) {
        QSizeF renderSize = document.pagePointSize(0);
        if (!renderSize.isValid() || renderSize.isEmpty()) {
            renderSize = QSizeF(720, 1040);
        } else {
            renderSize.scale(QSizeF(720, 1040), Qt::KeepAspectRatio);
        }
        data.cover = document.render(0, renderSize.toSize());
    }
    return data;
}

QString formatNameForSuffix(const QString &suffix)
{
    if (suffix == QLatin1String("md") || suffix == QLatin1String("markdown")) {
        return QStringLiteral("Markdown");
    }
    if (suffix == QLatin1String("htm") || suffix == QLatin1String("html")) {
        return QStringLiteral("HTML");
    }
    return suffix.toUpper();
}

} // namespace

BookMetadataService::BookMetadataService(BookCoverProvider *coverProvider)
    : m_coverProvider(coverProvider)
{
    Q_ASSERT(m_coverProvider);
}

BookMetadata BookMetadataService::inspect(const QUrl &sourceUrl) const
{
    BookMetadata metadata;
    if (!supports(sourceUrl)) {
        return metadata;
    }

    const QFileInfo fileInfo(sourceUrl.toLocalFile());
    const QString suffix = fileInfo.suffix().toLower();
    ExtractedBookData extracted;
    if (suffix == QLatin1String("fb2")) {
        extracted = extractFb2(fileInfo);
    } else if (suffix == QLatin1String("epub")) {
        extracted = extractEpub(fileInfo);
    } else if (suffix == QLatin1String("docx")) {
        extracted = extractDocx(fileInfo);
    } else if (suffix == QLatin1String("pdf")) {
        extracted = extractPdf(fileInfo);
    }

    metadata.title = normalizedText(extracted.title);
    metadata.author = normalizedText(extracted.author);
    metadata.hasDocumentTitle = !metadata.title.isEmpty();
    metadata.hasDocumentAuthor = !metadata.author.isEmpty();
    if (metadata.title.isEmpty()) {
        metadata.title = fileInfo.completeBaseName();
    }
    metadata.formatName = formatNameForSuffix(suffix);
    metadata.fingerprint = fingerprint(sourceUrl);
    metadata.coverUrl = extracted.cover.isNull()
                            ? m_coverProvider->cachedCover(metadata.fingerprint)
                            : m_coverProvider->storeCover(metadata.fingerprint,
                                                          extracted.cover);
    return metadata;
}

QString BookMetadataService::fingerprint(const QUrl &sourceUrl) const
{
    if (!sourceUrl.isLocalFile()) {
        return {};
    }

    const QFileInfo fileInfo(sourceUrl.toLocalFile());
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return {};
    }

    QString path = fileInfo.canonicalFilePath();
    if (path.isEmpty()) {
        path = fileInfo.absoluteFilePath();
    }
    QByteArray identity = path.toUtf8();
    identity.append('\0');
    identity.append(QByteArray::number(fileInfo.size()));
    identity.append('\0');
    identity.append(QByteArray::number(fileInfo.lastModified().toMSecsSinceEpoch()));
    return QString::fromLatin1(
        QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
}

bool BookMetadataService::supports(const QUrl &sourceUrl) const
{
    if (!sourceUrl.isLocalFile()) {
        return false;
    }
    const QFileInfo fileInfo(sourceUrl.toLocalFile());
    return fileInfo.exists()
           && fileInfo.isFile()
           && supportedSuffixes().contains(fileInfo.suffix().toLower());
}

QStringList BookMetadataService::supportedSuffixes()
{
    return {
        QStringLiteral("txt"),
        QStringLiteral("fb2"),
        QStringLiteral("epub"),
        QStringLiteral("pdf"),
        QStringLiteral("html"),
        QStringLiteral("htm"),
        QStringLiteral("md"),
        QStringLiteral("markdown"),
        QStringLiteral("docx")
    };
}
