#include "librarysearchmodel.h"

#include "../library/libraryrepository.h"

#include <QtConcurrentRun>
#include <QPointer>

#include <utility>

namespace {

constexpr int minimumQueryLength = 2;
constexpr int maximumSearchResults = 200;

} // namespace

LibrarySearchModel::LibrarySearchModel(LibraryRepository *repository,
                                       QString databaseFilePath,
                                       QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(repository)
    , m_databaseFilePath(std::move(databaseFilePath))
{
    Q_ASSERT(m_repository);
    connect(&m_indexWatcher,
            &QFutureWatcher<LibrarySearchIndexSummary>::finished,
            this,
            [this]() {
                const LibrarySearchIndexSummary summary = m_indexWatcher.result();
                applyIndexSummary(summary);
                if (!summary.errorMessage.isEmpty() || summary.canceled) {
                    m_indexStale = true;
                }
                m_indexing = false;
                m_cancellationRequested = false;
                emit indexingChanged();
                emit indexProgressChanged();
                refreshResults();

                const bool rebuild = m_rebuildPending;
                if (m_refreshPending || rebuild) {
                    m_refreshPending = false;
                    m_rebuildPending = false;
                    startIndexing(rebuild);
                }
            });

    m_totalBooks = m_repository->books().size();
    const LibrarySearchIndexSummary initialStatus =
        LibrarySearchIndex(m_databaseFilePath).status(m_totalBooks);
    applyIndexSummary(initialStatus);
    connect(m_repository,
            &LibraryRepository::changed,
            this,
            &LibrarySearchModel::markIndexStale);
}

LibrarySearchModel::~LibrarySearchModel()
{
    if (m_cancellation) {
        m_cancellation->store(true, std::memory_order_relaxed);
    }
    if (m_indexWatcher.isRunning()) {
        m_indexWatcher.waitForFinished();
    }
}

int LibrarySearchModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_hits.size();
}

QVariant LibrarySearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_hits.size()) {
        return {};
    }

    const LibrarySearchHit &hit = m_hits.at(index.row());
    switch (role) {
    case SourceUrlRole:
        return hit.sourceUrl;
    case BookTitleRole:
        return hit.bookTitle;
    case BookAuthorRole:
        return hit.bookAuthor;
    case FormatNameRole:
        return hit.formatName;
    case SnippetRole:
        return hit.snippet;
    case StartRole:
        return hit.start;
    case PageRole:
        return hit.page;
    case ProgressRole:
        return hit.progress;
    default:
        return {};
    }
}

QHash<int, QByteArray> LibrarySearchModel::roleNames() const
{
    return {{SourceUrlRole, "sourceUrl"},
            {BookTitleRole, "bookTitle"},
            {BookAuthorRole, "bookAuthor"},
            {FormatNameRole, "formatName"},
            {SnippetRole, "snippet"},
            {StartRole, "start"},
            {PageRole, "page"},
            {ProgressRole, "progress"}};
}

QString LibrarySearchModel::query() const
{
    return m_query;
}

bool LibrarySearchModel::indexing() const
{
    return m_indexing;
}

bool LibrarySearchModel::cancellationRequested() const
{
    return m_cancellationRequested;
}

int LibrarySearchModel::indexCompleted() const
{
    return m_indexCompleted;
}

qreal LibrarySearchModel::indexProgress() const
{
    return m_totalBooks > 0
               ? qreal(m_indexCompleted) / qreal(m_totalBooks)
               : 0;
}

int LibrarySearchModel::indexedBooks() const
{
    return m_indexedBooks;
}

int LibrarySearchModel::totalBooks() const
{
    return m_totalBooks;
}

int LibrarySearchModel::failedBooks() const
{
    return m_failedBooks;
}

QDateTime LibrarySearchModel::indexedAt() const
{
    return m_indexedAt;
}

QString LibrarySearchModel::errorMessage() const
{
    return m_errorMessage;
}

void LibrarySearchModel::setQuery(const QString &query)
{
    if (m_query == query) {
        return;
    }
    m_query = query;
    emit queryChanged();
    refreshResults();
}

QVariantMap LibrarySearchModel::get(int row) const
{
    if (row < 0 || row >= m_hits.size()) {
        return {};
    }
    return toVariantMap(m_hits.at(row));
}

void LibrarySearchModel::ensureIndexed()
{
    if (m_indexing) {
        return;
    }
    if (m_indexStale) {
        startIndexing(false);
    }
}

void LibrarySearchModel::rebuildIndex()
{
    if (m_indexing) {
        m_rebuildPending = true;
        return;
    }
    startIndexing(true);
}

void LibrarySearchModel::cancelIndexing()
{
    if (!m_indexing || m_cancellationRequested) {
        return;
    }
    m_refreshPending = false;
    m_rebuildPending = false;
    m_cancellationRequested = true;
    if (m_cancellation) {
        m_cancellation->store(true, std::memory_order_relaxed);
    }
    emit indexingChanged();
}

void LibrarySearchModel::clearError()
{
    setErrorMessage({});
}

QVariantMap LibrarySearchModel::toVariantMap(const LibrarySearchHit &hit)
{
    return {{QStringLiteral("sourceUrl"), hit.sourceUrl},
            {QStringLiteral("bookTitle"), hit.bookTitle},
            {QStringLiteral("bookAuthor"), hit.bookAuthor},
            {QStringLiteral("formatName"), hit.formatName},
            {QStringLiteral("snippet"), hit.snippet},
            {QStringLiteral("start"), hit.start},
            {QStringLiteral("page"), hit.page},
            {QStringLiteral("progress"), hit.progress}};
}

void LibrarySearchModel::markIndexStale()
{
    m_indexStale = true;
    if (m_indexing) {
        m_refreshPending = true;
    }
}

void LibrarySearchModel::startIndexing(bool rebuild)
{
    if (m_indexing) {
        m_refreshPending = true;
        m_rebuildPending = m_rebuildPending || rebuild;
        return;
    }

    const QVector<LibraryBook> books = m_repository->books();
    m_totalBooks = books.size();
    m_indexCompleted = 0;
    m_indexing = true;
    m_cancellationRequested = false;
    m_indexStale = false;
    m_cancellation = std::make_shared<std::atomic_bool>(false);
    setErrorMessage({});
    emit indexStatusChanged();
    emit indexingChanged();
    emit indexProgressChanged();

    const QString databaseFilePath = m_databaseFilePath;
    const std::shared_ptr<std::atomic_bool> cancellation = m_cancellation;
    const QPointer<LibrarySearchModel> model(this);
    m_indexWatcher.setFuture(QtConcurrent::run(
        [databaseFilePath, books, rebuild, cancellation, model]() {
            const auto progress = [model](int completed, int total) {
                if (!model) {
                    return;
                }
                QMetaObject::invokeMethod(
                    model,
                    [model, completed, total]() {
                        if (model) {
                            model->updateIndexProgress(completed, total);
                        }
                    },
                    Qt::QueuedConnection);
            };
            return LibrarySearchIndex(databaseFilePath)
                .synchronize(books, rebuild, cancellation.get(), progress);
        }));
}

void LibrarySearchModel::applyIndexSummary(const LibrarySearchIndexSummary &summary)
{
    m_totalBooks = summary.totalBooks;
    m_indexedBooks = summary.indexedBooks;
    m_failedBooks = summary.failedBooks;
    m_indexCompleted = summary.processedBooks;
    m_indexedAt = summary.indexedAt;
    setErrorMessage(summary.errorMessage.isEmpty()
                        ? QString()
                        : tr("Search index error: %1").arg(summary.errorMessage));
    emit indexStatusChanged();
}

void LibrarySearchModel::updateIndexProgress(int completed, int total)
{
    if (!m_indexing) {
        return;
    }
    const int boundedTotal = qMax(0, total);
    const int boundedCompleted = qBound(0, completed, boundedTotal);
    if (m_totalBooks == boundedTotal && m_indexCompleted == boundedCompleted) {
        return;
    }
    m_totalBooks = boundedTotal;
    m_indexCompleted = boundedCompleted;
    emit indexStatusChanged();
    emit indexProgressChanged();
}

void LibrarySearchModel::refreshResults()
{
    QVector<LibrarySearchHit> hits;
    QString searchError;
    if (m_query.trimmed().size() >= minimumQueryLength) {
        hits = LibrarySearchIndex(m_databaseFilePath)
                   .search(m_query, maximumSearchResults, &searchError);
    }

    beginResetModel();
    m_hits = std::move(hits);
    endResetModel();
    emit countChanged();
    if (!searchError.isEmpty()) {
        setErrorMessage(tr("Search index error: %1").arg(searchError));
    }
}

void LibrarySearchModel::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage == errorMessage) {
        return;
    }
    m_errorMessage = errorMessage;
    emit errorMessageChanged();
}
