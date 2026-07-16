#include "localdocumentloader.h"

#include "documentreader.h"
#include "documentreaders.h"
#include "document/documentlimits.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace {

[[maybe_unused]] const char *const loaderTranslationSources[] = {
    QT_TRANSLATE_NOOP("LocalDocumentLoader", "Only local files are supported."),
    QT_TRANSLATE_NOOP("LocalDocumentLoader", "File not found."),
    QT_TRANSLATE_NOOP("LocalDocumentLoader", "This book archive is too large. The limit is 2 GB."),
    QT_TRANSLATE_NOOP("LocalDocumentLoader", "Unsupported file format: .%1")
};

QString trLoader(const char *sourceText)
{
    return QCoreApplication::translate("LocalDocumentLoader", sourceText);
}

} // namespace

LocalDocumentLoader::LocalDocumentLoader()
{
    m_readers.emplace_back(std::make_unique<PlainTextDocumentReader>());
    m_readers.emplace_back(std::make_unique<Fb2DocumentReader>());
    m_readers.emplace_back(std::make_unique<EpubDocumentReader>());
    m_readers.emplace_back(std::make_unique<PdfDocumentReader>());
    m_readers.emplace_back(std::make_unique<HtmlDocumentReader>());
    m_readers.emplace_back(std::make_unique<MarkdownDocumentReader>());
    m_readers.emplace_back(std::make_unique<DocxDocumentReader>());
}

LocalDocumentLoader::~LocalDocumentLoader() = default;

DocumentLoadResult LocalDocumentLoader::load(const QUrl &fileUrl) const
{
    if (!fileUrl.isLocalFile()) {
        return DocumentLoadResult::failure(trLoader("Only local files are supported."));
    }

    const QFileInfo fileInfo(fileUrl.toLocalFile());
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return DocumentLoadResult::failure(trLoader("File not found."));
    }

    const QString suffix = fileInfo.suffix().toLower();
    if ((suffix == QLatin1String("epub") || suffix == QLatin1String("docx"))
        && fileInfo.size() > DocumentLimits::maximumArchiveContainerBytes) {
        return DocumentLoadResult::failure(
            trLoader("This book archive is too large. The limit is 2 GB."));
    }

    for (const std::unique_ptr<DocumentReader> &reader : m_readers) {
        if (reader->supportsSuffix(suffix)) {
            return reader->load(fileInfo);
        }
    }

    return DocumentLoadResult::failure(trLoader("Unsupported file format: .%1").arg(suffix));
}
