#include "libraryrepository.h"

#include "bookmetadataservice.h"
#include "../storage/documentstoragekey.h"
#include "../storage/localstatestore.h"

#include <QFileInfo>

#include <algorithm>
#include <utility>

LibraryRepository::LibraryRepository(LocalStateStore *store,
                                     BookMetadataService *metadataService,
                                     QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_metadataService(metadataService)
{
    Q_ASSERT(m_store);
    Q_ASSERT(m_metadataService);

    connect(m_store,
            &LocalStateStore::libraryChanged,
            this,
            [this]() {
                if (m_batchDepth > 0) {
                    m_changePending = true;
                } else {
                    emit changed();
                }
            });
    connect(m_store,
            &LocalStateStore::documentProgressChanged,
            this,
            &LibraryRepository::documentProgressChanged);
}

QVector<LibraryBook> LibraryRepository::books()
{
    return m_store->libraryBooks();
}

std::optional<LibraryBook> LibraryRepository::book(const QUrl &sourceUrl)
{
    const QVector<LibraryBook> libraryBooks = books();
    const auto match = std::find_if(libraryBooks.cbegin(),
                                    libraryBooks.cend(),
                                    [&sourceUrl](const LibraryBook &book) {
                                        return book.sourceUrl == sourceUrl;
                                    });
    return match == libraryBooks.cend() ? std::nullopt
                                        : std::optional<LibraryBook>(*match);
}

QString LibraryRepository::sortMode() const
{
    return m_store->librarySortMode();
}

QString LibraryRepository::viewMode() const
{
    return m_store->libraryViewMode();
}

void LibraryRepository::setSortMode(const QString &sortMode)
{
    m_store->setLibrarySortMode(sortMode);
}

void LibraryRepository::setViewMode(const QString &viewMode)
{
    m_store->setLibraryViewMode(viewMode);
}

void LibraryRepository::recordBookOpened(const QUrl &sourceUrl,
                                         const QString &title,
                                         const QString &author,
                                         const QString &formatName)
{
    m_store->recordBookOpened(sourceUrl, title, author, formatName);
}

bool LibraryRepository::registerBook(const QUrl &sourceUrl,
                                     const QString &collectionPath,
                                     bool restoreExcluded,
                                     bool inspectMetadata)
{
    if (!m_metadataService->supports(sourceUrl)) {
        return false;
    }
    if (m_store->hasLibraryRecord(sourceUrl)) {
        if (m_store->containsLibraryBook(sourceUrl)) {
            m_store->setBookCollection(sourceUrl, collectionPath);
        } else if (restoreExcluded) {
            const QFileInfo fileInfo(sourceUrl.toLocalFile());
            const BookMetadata metadata = inspectMetadata
                                              ? m_metadataService->inspect(sourceUrl)
                                              : BookMetadata{
                                                    fileInfo.completeBaseName(),
                                                    QString(),
                                                    fileInfo.suffix().toUpper()};
            m_store->recordBookOpened(sourceUrl,
                                      metadata.title,
                                      metadata.author,
                                      metadata.formatName);
            m_store->setBookCollection(sourceUrl, collectionPath);
            m_store->updateBookMetadata(sourceUrl,
                                        metadata.title,
                                        metadata.author,
                                        metadata.formatName,
                                        metadata.coverUrl,
                                        metadata.fingerprint);
            return true;
        }
        return false;
    }

    const QFileInfo fileInfo(sourceUrl.toLocalFile());
    const BookMetadata metadata = inspectMetadata
                                      ? m_metadataService->inspect(sourceUrl)
                                      : BookMetadata{fileInfo.completeBaseName(),
                                                     QString(),
                                                     fileInfo.suffix().toUpper()};
    m_store->registerLibraryBook(sourceUrl,
                                 metadata.title,
                                 metadata.author,
                                 metadata.formatName,
                                 metadata.coverUrl,
                                 metadata.fingerprint,
                                 collectionPath);
    return true;
}

bool LibraryRepository::supports(const QUrl &sourceUrl) const
{
    return m_metadataService->supports(sourceUrl);
}

void LibraryRepository::removeBook(const QUrl &sourceUrl)
{
    m_store->removeFromLibrary(sourceUrl);
}

void LibraryRepository::setBookCollection(const QUrl &sourceUrl,
                                          const QString &collectionPath)
{
    m_store->setBookCollection(sourceUrl, collectionPath);
}

bool LibraryRepository::relinkBook(const QUrl &oldSourceUrl,
                                   const QUrl &newSourceUrl,
                                   QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    };

    if (oldSourceUrl.isEmpty()) {
        return fail(tr("The missing library item is no longer available."));
    }
    if (!newSourceUrl.isLocalFile()) {
        return fail(tr("Choose a local book file."));
    }

    const QFileInfo fileInfo(newSourceUrl.toLocalFile());
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return fail(tr("The selected file does not exist."));
    }

    const QUrl normalizedUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    if (!m_metadataService->supports(normalizedUrl)) {
        return fail(tr("The selected file format is not supported."));
    }
    if (DocumentStorageKey::id(oldSourceUrl) != DocumentStorageKey::id(normalizedUrl)
        && m_store->containsLibraryBook(normalizedUrl)) {
        return fail(tr("This book is already in your library."));
    }
    if (!m_store->relinkDocument(oldSourceUrl, normalizedUrl)) {
        return fail(tr("Could not update the library item."));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

bool LibraryRepository::updateBookDetails(const QVector<QUrl> &sourceUrls,
                                          const BookMetadataPatch &patch,
                                          QString *errorMessage)
{
    if (sourceUrls.isEmpty()) {
        if (errorMessage) {
            *errorMessage = tr("Select at least one book.");
        }
        return false;
    }
    return m_store->updateBookDetails(sourceUrls, patch, errorMessage);
}

bool LibraryRepository::setCustomCover(const QUrl &sourceUrl,
                                       const QUrl &imageUrl,
                                       QString *errorMessage)
{
    if (!m_store->containsLibraryBook(sourceUrl)) {
        if (errorMessage) {
            *errorMessage = tr("The selected book is no longer in the library.");
        }
        return false;
    }

    QUrl coverUrl;
    if (!imageUrl.isEmpty()) {
        coverUrl = m_customCoverStore.importCover(sourceUrl, imageUrl, errorMessage);
        if (coverUrl.isEmpty()) {
            return false;
        }
    }
    BookMetadataPatch patch;
    patch.customCoverUrl = coverUrl;
    return m_store->updateBookDetails({sourceUrl}, patch, errorMessage);
}

void LibraryRepository::persistScannedMetadata(const QUrl &sourceUrl,
                                                const BookMetadata &metadata)
{
    if (sourceUrl.isEmpty() || metadata.fingerprint.isEmpty()) {
        return;
    }
    m_store->updateBookMetadata(sourceUrl,
                                metadata.title,
                                metadata.author,
                                metadata.formatName,
                                metadata.coverUrl,
                                metadata.fingerprint);
}

void LibraryRepository::beginBatchUpdate()
{
    ++m_batchDepth;
}

void LibraryRepository::endBatchUpdate()
{
    Q_ASSERT(m_batchDepth > 0);
    if (m_batchDepth <= 0) {
        return;
    }
    --m_batchDepth;
    if (m_batchDepth == 0 && std::exchange(m_changePending, false)) {
        emit changed();
    }
}

void LibraryRepository::setManagedRootPath(const QString &rootPath)
{
    m_customCoverStore.setManagedRootPath(rootPath);
}
