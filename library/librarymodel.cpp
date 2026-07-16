#include "librarymodel.h"

#include "libraryrepository.h"

#include <QtGlobal>

#include <algorithm>
#include <utility>

namespace {

constexpr qreal unreadProgressThreshold = 0.001;
constexpr qreal finishedProgressThreshold = 0.995;

int compareText(const QString &left, const QString &right)
{
    return QString::localeAwareCompare(left.trimmed(), right.trimmed());
}

} // namespace

LibraryModel::LibraryModel(LibraryRepository *repository, QObject *parent)
    : QAbstractListModel(parent)
    , m_repository(repository)
{
    Q_ASSERT(m_repository);
    m_sortMode = m_repository->sortMode();
    m_viewMode = m_repository->viewMode();

    connect(m_repository, &LibraryRepository::changed, this, &LibraryModel::refresh);
    connect(m_repository,
            &LibraryRepository::documentProgressChanged,
            this,
            &LibraryModel::updateProgress);
    refresh();
}

int LibraryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_visibleBooks.size();
}

QVariant LibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visibleBooks.size()) {
        return {};
    }

    const LibraryBook &book = m_visibleBooks.at(index.row());
    switch (role) {
    case SourceUrlRole:
        return book.sourceUrl;
    case SourcePathRole:
        return book.sourcePath;
    case TitleRole:
        return book.title;
    case AuthorRole:
        return book.author;
    case FormatNameRole:
        return book.formatName;
    case CoverUrlRole:
        return book.coverUrl;
    case ProgressRole:
        return book.progress;
    case LastOpenedRole:
        return book.lastOpened;
    case FileAvailableRole:
        return book.fileAvailable;
    default:
        return {};
    }
}

QHash<int, QByteArray> LibraryModel::roleNames() const
{
    return {
        {SourceUrlRole, "sourceUrl"},
        {SourcePathRole, "sourcePath"},
        {TitleRole, "title"},
        {AuthorRole, "author"},
        {FormatNameRole, "formatName"},
        {CoverUrlRole, "coverUrl"},
        {ProgressRole, "progress"},
        {LastOpenedRole, "lastOpened"},
        {FileAvailableRole, "fileAvailable"}
    };
}

QString LibraryModel::filterText() const
{
    return m_filterText;
}

QString LibraryModel::sortMode() const
{
    return m_sortMode;
}

QString LibraryModel::formatFilter() const
{
    return m_formatFilter;
}

QString LibraryModel::progressFilter() const
{
    return m_progressFilter;
}

QString LibraryModel::viewMode() const
{
    return m_viewMode;
}

QStringList LibraryModel::availableFormats() const
{
    return m_availableFormats;
}

bool LibraryModel::hasActiveFilters() const
{
    return !m_filterText.isEmpty()
           || m_formatFilter != QLatin1String("all")
           || m_progressFilter != QLatin1String("all");
}

QString LibraryModel::errorMessage() const
{
    return m_errorMessage;
}

int LibraryModel::totalCount() const
{
    return m_allBooks.size();
}

QVariantMap LibraryModel::recentBook() const
{
    return m_allBooks.isEmpty() ? QVariantMap() : toVariantMap(m_allBooks.constFirst());
}

void LibraryModel::setFilterText(const QString &filterText)
{
    const QString normalizedFilter = filterText.trimmed();
    if (m_filterText == normalizedFilter) {
        return;
    }

    const bool hadActiveFilters = hasActiveFilters();
    m_filterText = normalizedFilter;
    emit filterTextChanged();
    rebuildVisibleBooks();
    if (hadActiveFilters != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
}

void LibraryModel::setSortMode(const QString &sortMode)
{
    const QString normalizedMode = sortMode == QLatin1String("title")
                                       || sortMode == QLatin1String("author")
                                       ? sortMode
                                       : QStringLiteral("recent");
    if (m_sortMode == normalizedMode) {
        return;
    }

    m_sortMode = normalizedMode;
    m_repository->setSortMode(normalizedMode);
    emit sortModeChanged();
    rebuildVisibleBooks();
}

void LibraryModel::setFormatFilter(const QString &formatFilter)
{
    const QString normalizedFilter = formatFilter.trimmed().isEmpty()
                                         ? QStringLiteral("all")
                                         : formatFilter.trimmed();
    if (m_formatFilter.compare(normalizedFilter, Qt::CaseInsensitive) == 0) {
        return;
    }

    const bool hadActiveFilters = hasActiveFilters();
    m_formatFilter = normalizedFilter;
    emit formatFilterChanged();
    rebuildVisibleBooks();
    if (hadActiveFilters != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
}

void LibraryModel::setProgressFilter(const QString &progressFilter)
{
    const QString normalizedFilter = progressFilter == QLatin1String("unread")
                                             || progressFilter == QLatin1String("reading")
                                             || progressFilter == QLatin1String("finished")
                                         ? progressFilter
                                         : QStringLiteral("all");
    if (m_progressFilter == normalizedFilter) {
        return;
    }

    const bool hadActiveFilters = hasActiveFilters();
    m_progressFilter = normalizedFilter;
    emit progressFilterChanged();
    rebuildVisibleBooks();
    if (hadActiveFilters != hasActiveFilters()) {
        emit hasActiveFiltersChanged();
    }
}

void LibraryModel::setViewMode(const QString &viewMode)
{
    const QString normalizedMode = viewMode == QLatin1String("list")
                                       ? QStringLiteral("list")
                                       : QStringLiteral("grid");
    if (m_viewMode == normalizedMode) {
        return;
    }

    m_viewMode = normalizedMode;
    m_repository->setViewMode(normalizedMode);
    emit viewModeChanged();
}

void LibraryModel::refresh()
{
    const int previousTotalCount = m_allBooks.size();
    m_allBooks = m_repository->books();

    rebuildAvailableFormats();
    rebuildVisibleBooks();
    if (previousTotalCount != m_allBooks.size()) {
        emit totalCountChanged();
    }
    emit recentBookChanged();
}

void LibraryModel::removeBook(int row)
{
    if (row < 0 || row >= m_visibleBooks.size()) {
        return;
    }
    m_repository->removeBook(m_visibleBooks.at(row).sourceUrl);
}

bool LibraryModel::relinkBook(const QUrl &oldSourceUrl, const QUrl &newSourceUrl)
{
    QString error;
    if (!m_repository->relinkBook(oldSourceUrl, newSourceUrl, &error)) {
        setErrorMessage(error);
        return false;
    }
    setErrorMessage({});
    return true;
}

void LibraryModel::clearFilters()
{
    const bool changed = !m_filterText.isEmpty()
                         || m_formatFilter != QLatin1String("all")
                         || m_progressFilter != QLatin1String("all");
    if (!changed) {
        return;
    }

    if (!m_filterText.isEmpty()) {
        m_filterText.clear();
        emit filterTextChanged();
    }
    if (m_formatFilter != QLatin1String("all")) {
        m_formatFilter = QStringLiteral("all");
        emit formatFilterChanged();
    }
    if (m_progressFilter != QLatin1String("all")) {
        m_progressFilter = QStringLiteral("all");
        emit progressFilterChanged();
    }
    rebuildVisibleBooks();
    emit hasActiveFiltersChanged();
}

void LibraryModel::clearError()
{
    setErrorMessage({});
}

QVariantMap LibraryModel::toVariantMap(const LibraryBook &book)
{
    return {
        {QStringLiteral("sourceUrl"), book.sourceUrl},
        {QStringLiteral("sourcePath"), book.sourcePath},
        {QStringLiteral("title"), book.title},
        {QStringLiteral("author"), book.author},
        {QStringLiteral("formatName"), book.formatName},
        {QStringLiteral("coverUrl"), book.coverUrl},
        {QStringLiteral("progress"), book.progress},
        {QStringLiteral("lastOpened"), book.lastOpened},
        {QStringLiteral("fileAvailable"), book.fileAvailable}
    };
}

bool LibraryModel::matchesFilters(const LibraryBook &book) const
{
    const bool matchesText = m_filterText.isEmpty()
                             || book.title.contains(m_filterText, Qt::CaseInsensitive)
                             || book.author.contains(m_filterText, Qt::CaseInsensitive)
                             || book.formatName.contains(m_filterText, Qt::CaseInsensitive)
                             || book.sourcePath.contains(m_filterText, Qt::CaseInsensitive);
    const bool matchesFormat = m_formatFilter == QLatin1String("all")
                               || book.formatName.compare(m_formatFilter,
                                                          Qt::CaseInsensitive) == 0;

    bool matchesProgress = true;
    if (m_progressFilter == QLatin1String("unread")) {
        matchesProgress = book.progress <= unreadProgressThreshold;
    } else if (m_progressFilter == QLatin1String("reading")) {
        matchesProgress = book.progress > unreadProgressThreshold
                          && book.progress < finishedProgressThreshold;
    } else if (m_progressFilter == QLatin1String("finished")) {
        matchesProgress = book.progress >= finishedProgressThreshold;
    }
    return matchesText && matchesFormat && matchesProgress;
}

void LibraryModel::rebuildVisibleBooks()
{
    const int previousCount = m_visibleBooks.size();

    beginResetModel();
    m_visibleBooks.clear();
    m_visibleBooks.reserve(m_allBooks.size());
    for (const LibraryBook &book : std::as_const(m_allBooks)) {
        if (matchesFilters(book)) {
            m_visibleBooks.append(book);
        }
    }

    if (m_sortMode == QLatin1String("title")) {
        std::stable_sort(m_visibleBooks.begin(),
                         m_visibleBooks.end(),
                         [](const LibraryBook &left, const LibraryBook &right) {
                             return compareText(left.title, right.title) < 0;
                         });
    } else if (m_sortMode == QLatin1String("author")) {
        std::stable_sort(m_visibleBooks.begin(),
                         m_visibleBooks.end(),
                         [](const LibraryBook &left, const LibraryBook &right) {
                             const QString leftAuthor = left.author.isEmpty()
                                                            ? left.title
                                                            : left.author;
                             const QString rightAuthor = right.author.isEmpty()
                                                             ? right.title
                                                             : right.author;
                             const int authorOrder = compareText(leftAuthor, rightAuthor);
                             return authorOrder == 0
                                        ? compareText(left.title, right.title) < 0
                                        : authorOrder < 0;
                         });
    }
    endResetModel();

    if (previousCount != m_visibleBooks.size()) {
        emit countChanged();
    }
}

void LibraryModel::rebuildAvailableFormats()
{
    QStringList formats;
    for (const LibraryBook &book : std::as_const(m_allBooks)) {
        if (!book.formatName.isEmpty()
            && !formats.contains(book.formatName, Qt::CaseInsensitive)) {
            formats.append(book.formatName);
        }
    }
    std::sort(formats.begin(), formats.end(), [](const QString &left, const QString &right) {
        return compareText(left, right) < 0;
    });
    if (m_availableFormats != formats) {
        m_availableFormats = formats;
        emit availableFormatsChanged();
    }
}

void LibraryModel::updateProgress(const QUrl &sourceUrl, qreal progress)
{
    progress = qBound(qreal(0), progress, qreal(1));
    bool recentBookUpdated = false;
    for (qsizetype row = 0; row < m_allBooks.size(); ++row) {
        LibraryBook &book = m_allBooks[row];
        if (book.sourceUrl == sourceUrl) {
            book.progress = progress;
            recentBookUpdated = row == 0;
            break;
        }
    }
    rebuildVisibleBooks();
    if (recentBookUpdated) {
        emit recentBookChanged();
    }
}

void LibraryModel::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage == errorMessage) {
        return;
    }
    m_errorMessage = errorMessage;
    emit errorMessageChanged();
}
