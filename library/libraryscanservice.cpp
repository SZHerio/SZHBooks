#include "libraryscanservice.h"

#include "bookmetadataservice.h"
#include "cloudfileavailability.h"

#include <QFileInfo>
#include <QThread>
#include <QtConcurrentMap>

#include <algorithm>
#include <utility>

namespace {

constexpr int resultBatchSize = 32;

} // namespace

LibraryScanService::LibraryScanService(const BookMetadataService *metadataService,
                                       QObject *parent)
    : QObject(parent)
    , m_metadataService(metadataService)
{
    Q_ASSERT(m_metadataService);
    qRegisterMetaType<LibraryScanResult>();
    m_threadPool.setMaxThreadCount(qBound(1, QThread::idealThreadCount() / 2, 4));
    m_threadPool.setExpiryTimeout(5000);

    m_resultTimer.setSingleShot(true);
    m_resultTimer.setInterval(0);
    connect(&m_resultTimer,
            &QTimer::timeout,
            this,
            &LibraryScanService::flushResults);
    connect(&m_watcher,
            &QFutureWatcher<LibraryScanResult>::resultsReadyAt,
            this,
            &LibraryScanService::queueResults);
    connect(&m_watcher,
            &QFutureWatcher<LibraryScanResult>::finished,
            this,
            [this]() {
                m_workerFinished = true;
                flushResults();
            });
}

LibraryScanService::~LibraryScanService()
{
    m_restartPending = false;
    if (m_watcher.isRunning()) {
        m_watcher.cancel();
        m_watcher.waitForFinished();
    }
}

bool LibraryScanService::scanning() const
{
    return m_scanning;
}

bool LibraryScanService::cancellationRequested() const
{
    return m_cancellationRequested;
}

int LibraryScanService::completed() const
{
    return m_completed;
}

int LibraryScanService::total() const
{
    return m_total;
}

qreal LibraryScanService::progress() const
{
    return m_total > 0 ? qreal(m_completed) / qreal(m_total) : 0;
}

void LibraryScanService::start(const QVector<LibraryBook> &books)
{
    if (m_scanning) {
        m_restartBooks = books;
        m_restartPending = true;
        if (!m_cancellationRequested) {
            m_cancellationRequested = true;
            emit scanningChanged();
            m_watcher.cancel();
        }
        return;
    }

    m_pendingResults.clear();
    m_pendingResultOffset = 0;
    m_completed = 0;
    m_total = books.size();
    m_cancellationRequested = false;
    m_workerFinished = false;
    emit progressChanged();

    if (books.isEmpty()) {
        emit finished(false);
        return;
    }

    m_scanning = true;
    emit scanningChanged();
    const BookMetadataService *metadataService = m_metadataService;
    m_watcher.setFuture(QtConcurrent::mapped(
        &m_threadPool,
        books,
        [metadataService](const LibraryBook &book) {
            return scanBook(book, metadataService);
        }));
}

void LibraryScanService::cancel()
{
    if (!m_scanning || m_cancellationRequested) {
        return;
    }
    m_restartBooks.clear();
    m_restartPending = false;
    m_cancellationRequested = true;
    emit scanningChanged();
    m_watcher.cancel();
}

LibraryScanResult LibraryScanService::scanBook(
    const LibraryBook &book,
    const BookMetadataService *metadataService)
{
    LibraryScanResult result;
    result.sourceUrl = book.sourceUrl;
    if (!book.sourceUrl.isLocalFile()) {
        result.fileAvailable = !book.sourceUrl.isEmpty();
        return result;
    }

    const QString filePath = book.sourceUrl.toLocalFile();
    const QFileInfo fileInfo(filePath);
    result.fileAvailable = fileInfo.exists() && fileInfo.isFile();
    if (!result.fileAvailable) {
        return result;
    }

    result.cloudPlaceholder = CloudFileAvailability::isOnlineOnly(filePath);
    if (result.cloudPlaceholder) {
        return result;
    }

    const QString fingerprint = metadataService->fingerprint(book.sourceUrl);
    const bool automaticCoverMissing = !book.coverUrl.isEmpty()
                                       && book.coverUrl != book.customCoverUrl
                                       && book.coverUrl.isLocalFile()
                                       && !QFileInfo::exists(book.coverUrl.toLocalFile());
    if (fingerprint.isEmpty()
        || (fingerprint == book.metadataFingerprint && !automaticCoverMissing)) {
        return result;
    }

    result.metadata = metadataService->inspect(book.sourceUrl);
    result.metadataInspected = !result.metadata.fingerprint.isEmpty();
    return result;
}

void LibraryScanService::queueResults(int begin, int end)
{
    for (int index = begin; index < end; ++index) {
        m_pendingResults.append(m_watcher.resultAt(index));
    }
    m_completed += end - begin;
    emit progressChanged();
    if (!m_resultTimer.isActive()) {
        m_resultTimer.start();
    }
}

void LibraryScanService::flushResults()
{
    const qsizetype available = m_pendingResults.size() - m_pendingResultOffset;
    if (available > 0) {
        const qsizetype count = std::min<qsizetype>(resultBatchSize, available);
        emit resultsReady(m_pendingResults.mid(m_pendingResultOffset, count));
        m_pendingResultOffset += count;
    }

    if (m_pendingResultOffset < m_pendingResults.size()) {
        m_resultTimer.start();
        return;
    }

    m_pendingResults.clear();
    m_pendingResultOffset = 0;
    if (m_workerFinished) {
        completeRun();
    }
}

void LibraryScanService::completeRun()
{
    const bool canceled = m_cancellationRequested || m_completed < m_total;
    m_workerFinished = false;
    m_scanning = false;
    m_cancellationRequested = false;
    emit scanningChanged();
    emit finished(canceled);

    if (!m_restartPending) {
        return;
    }
    const QVector<LibraryBook> books = std::exchange(m_restartBooks, {});
    m_restartPending = false;
    QTimer::singleShot(0, this, [this, books]() { start(books); });
}
