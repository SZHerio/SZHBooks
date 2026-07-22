#include "librarysearchindex.h"

#include "../documentloadresult.h"
#include "../localdocumentloader.h"

#include <QDir>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QRegularExpression>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUuid>

#include <algorithm>
#include <utility>

namespace {

constexpr int searchSchemaVersion = 1;
constexpr int targetChunkLength = 2200;
constexpr int minimumChunkLength = 900;
constexpr int maximumSnippetLength = 260;

struct SearchChunk final
{
    QString body;
    int start = -1;
    int page = -1;
    qreal progress = 0;
};

class SearchDatabase final
{
public:
    explicit SearchDatabase(const QString &filePath)
        : m_connectionName(QStringLiteral("szhbooks-search-%1-%2")
                               .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                               .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
        , m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName))
    {
        const QFileInfo fileInfo(filePath);
        if (!QDir().mkpath(fileInfo.absolutePath())) {
            m_errorMessage = QStringLiteral("Could not create the search index folder.");
            return;
        }

        m_database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
        m_database.setDatabaseName(filePath);
        if (!m_database.open()) {
            m_errorMessage = m_database.lastError().text();
            return;
        }

        QSqlQuery pragma(m_database);
        pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
        pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
        m_valid = prepareSchema();
    }

    ~SearchDatabase()
    {
        m_database.close();
        m_database = {};
        QSqlDatabase::removeDatabase(m_connectionName);
    }

    bool isValid() const { return m_valid; }
    bool ftsAvailable() const { return m_ftsAvailable; }
    QString errorMessage() const { return m_errorMessage; }
    QSqlDatabase &database() { return m_database; }

private:
    bool execute(const QString &statement)
    {
        QSqlQuery query(m_database);
        if (query.exec(statement)) {
            return true;
        }
        m_errorMessage = query.lastError().text();
        return false;
    }

    bool prepareSchema()
    {
        if (!execute(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS search_metadata ("
                "key TEXT PRIMARY KEY, value TEXT NOT NULL)"))) {
            return false;
        }

        int storedVersion = 0;
        {
            QSqlQuery versionQuery(m_database);
            versionQuery.prepare(QStringLiteral(
                "SELECT value FROM search_metadata WHERE key = 'schema_version'"));
            if (versionQuery.exec() && versionQuery.next()) {
                storedVersion = versionQuery.value(0).toInt();
            }
        }

        if (storedVersion != 0 && storedVersion != searchSchemaVersion) {
            if (!execute(QStringLiteral("DROP TABLE IF EXISTS content_fts"))
                || !execute(QStringLiteral("DROP TABLE IF EXISTS content_chunks"))
                || !execute(QStringLiteral("DROP TABLE IF EXISTS indexed_books"))) {
                return false;
            }
        }

        if (!execute(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS indexed_books ("
                "source_url TEXT PRIMARY KEY, file_size INTEGER NOT NULL, "
                "modified_msecs INTEGER NOT NULL, title TEXT NOT NULL, "
                "author TEXT NOT NULL, format_name TEXT NOT NULL, "
                "indexed_at TEXT NOT NULL, success INTEGER NOT NULL, "
                "error_message TEXT NOT NULL)"))
            || !execute(QStringLiteral(
                "CREATE TABLE IF NOT EXISTS content_chunks ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, source_url TEXT NOT NULL, "
                "text_start INTEGER NOT NULL, page INTEGER NOT NULL, "
                "progress REAL NOT NULL, body TEXT NOT NULL, "
                "title_terms TEXT NOT NULL, author_terms TEXT NOT NULL)"))
            || !execute(QStringLiteral(
                "CREATE INDEX IF NOT EXISTS content_chunks_source_idx "
                "ON content_chunks(source_url)"))) {
            return false;
        }

        QSqlQuery ftsQuery(m_database);
        m_ftsAvailable = ftsQuery.exec(QStringLiteral(
            "CREATE VIRTUAL TABLE IF NOT EXISTS content_fts USING fts5("
            "source_url UNINDEXED, text_start UNINDEXED, page UNINDEXED, "
            "progress UNINDEXED, body, title_terms, author_terms, "
            "tokenize='unicode61 remove_diacritics 2')"));

        QSqlQuery versionUpdate(m_database);
        versionUpdate.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO search_metadata(key, value) VALUES('schema_version', ?)"));
        versionUpdate.addBindValue(searchSchemaVersion);
        if (!versionUpdate.exec()) {
            m_errorMessage = versionUpdate.lastError().text();
            return false;
        }
        return true;
    }

    QString m_connectionName;
    QSqlDatabase m_database;
    QString m_errorMessage;
    bool m_valid = false;
    bool m_ftsAvailable = false;
};

QString sourceKey(const QUrl &sourceUrl)
{
    return sourceUrl.toString(QUrl::FullyEncoded);
}

QString databaseText(const QString &value)
{
    return value.isNull() ? QStringLiteral("") : value;
}

QString localPath(const LibraryBook &book)
{
    return book.sourcePath.isEmpty() ? book.sourceUrl.toLocalFile() : book.sourcePath;
}

QVector<QString> queryTokens(const QString &query)
{
    static const QRegularExpression tokenPattern(QStringLiteral("[\\p{L}\\p{N}_]+"));
    QVector<QString> tokens;
    QSet<QString> uniqueTokens;
    auto iterator = tokenPattern.globalMatch(query.toCaseFolded());
    while (iterator.hasNext()) {
        const QString token = iterator.next().captured();
        if (token.isEmpty() || uniqueTokens.contains(token)) {
            continue;
        }
        uniqueTokens.insert(token);
        tokens.append(token);
    }
    return tokens;
}

QString normalizedSnippet(QString text)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    text.replace(whitespace, QStringLiteral(" "));
    return text.trimmed();
}

QString fallbackSnippet(const QString &body, const QVector<QString> &tokens)
{
    if (body.isEmpty()) {
        return {};
    }

    int matchPosition = -1;
    for (const QString &token : tokens) {
        const int position = body.indexOf(token, 0, Qt::CaseInsensitive);
        if (position >= 0 && (matchPosition < 0 || position < matchPosition)) {
            matchPosition = position;
        }
    }
    if (matchPosition < 0) {
        matchPosition = 0;
    }

    const int start = qMax(0, matchPosition - maximumSnippetLength / 3);
    const int length = qMin(maximumSnippetLength, body.size() - start);
    QString snippet = normalizedSnippet(body.mid(start, length));
    if (start > 0) {
        snippet.prepend(QStringLiteral("... "));
    }
    if (start + length < body.size()) {
        snippet.append(QStringLiteral(" ..."));
    }
    return snippet;
}

int preferredChunkEnd(const QString &text, int start, int tentativeEnd)
{
    if (tentativeEnd >= text.size()) {
        return text.size();
    }

    const int earliestBreak = qMin(tentativeEnd, start + minimumChunkLength);
    const QChar separators[] = {QChar(QChar::ParagraphSeparator), QChar('\n')};
    for (const QChar separator : separators) {
        const int position = text.lastIndexOf(separator, tentativeEnd);
        if (position >= earliestBreak) {
            return position + 1;
        }
    }

    const int space = text.lastIndexOf(QChar(' '), tentativeEnd);
    return space >= earliestBreak ? space + 1 : tentativeEnd;
}

QVector<SearchChunk> chunksForText(const QString &text)
{
    QVector<SearchChunk> chunks;
    int start = 0;
    while (start < text.size()) {
        const int tentativeEnd = qMin(text.size(), start + targetChunkLength);
        const int end = preferredChunkEnd(text, start, tentativeEnd);
        const QString body = text.mid(start, end - start);
        if (!body.trimmed().isEmpty()) {
            chunks.append({body,
                           start,
                           -1,
                           text.size() > 1
                               ? static_cast<qreal>(start) / (text.size() - 1)
                               : 0});
        }
        start = qMax(end, start + 1);
    }
    return chunks;
}

QVector<SearchChunk> chunksForPdf(const QString &filePath, QString *errorMessage)
{
    QPdfDocument document;
    const QPdfDocument::Error loadError = document.load(filePath);
    if (loadError != QPdfDocument::Error::None) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("PDF text could not be read (error %1).")
                                .arg(static_cast<int>(loadError));
        }
        return {};
    }

    QVector<SearchChunk> chunks;
    const int pageCount = document.pageCount();
    for (int page = 0; page < pageCount; ++page) {
        const QString text = document.getAllText(page).text();
        if (text.trimmed().isEmpty()) {
            continue;
        }
        chunks.append({text,
                       -1,
                       page,
                       pageCount > 1
                           ? static_cast<qreal>(page) / (pageCount - 1)
                           : 0});
    }
    return chunks;
}

QVector<SearchChunk> loadChunks(const LibraryBook &book, QString *errorMessage)
{
    LocalDocumentLoader loader;
    const DocumentLoadResult document = loader.load(book.sourceUrl);
    if (!document.isSuccess()) {
        if (errorMessage) {
            *errorMessage = document.errorMessage;
        }
        return {};
    }

    if (document.viewMode == DocumentViewMode::Pdf) {
        return chunksForPdf(localPath(book), errorMessage);
    }
    return chunksForText(document.text);
}

bool clearBookRows(QSqlDatabase &database,
                   const QString &key,
                   bool ftsAvailable,
                   QString *errorMessage)
{
    QSqlQuery chunksQuery(database);
    chunksQuery.prepare(QStringLiteral("DELETE FROM content_chunks WHERE source_url = ?"));
    chunksQuery.addBindValue(key);
    if (!chunksQuery.exec()) {
        if (errorMessage) {
            *errorMessage = chunksQuery.lastError().text();
        }
        return false;
    }

    if (ftsAvailable) {
        QSqlQuery ftsQuery(database);
        ftsQuery.prepare(QStringLiteral("DELETE FROM content_fts WHERE source_url = ?"));
        ftsQuery.addBindValue(key);
        if (!ftsQuery.exec()) {
            if (errorMessage) {
                *errorMessage = ftsQuery.lastError().text();
            }
            return false;
        }
    }
    return true;
}

bool storeBook(QSqlDatabase &database,
               const LibraryBook &book,
               const QFileInfo &fileInfo,
               const QVector<SearchChunk> &chunks,
               const QString &loadError,
               bool ftsAvailable,
               QString *errorMessage)
{
    if (!database.transaction()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        return false;
    }

    const QString key = sourceKey(book.sourceUrl);
    if (!clearBookRows(database, key, ftsAvailable, errorMessage)) {
        database.rollback();
        return false;
    }

    const QString indexedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QSqlQuery bookQuery(database);
    bookQuery.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO indexed_books("
        "source_url, file_size, modified_msecs, title, author, format_name, "
        "indexed_at, success, error_message) VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    bookQuery.addBindValue(key);
    bookQuery.addBindValue(fileInfo.size());
    bookQuery.addBindValue(fileInfo.lastModified().toMSecsSinceEpoch());
    bookQuery.addBindValue(databaseText(book.title));
    bookQuery.addBindValue(databaseText(book.author));
    bookQuery.addBindValue(databaseText(book.formatName));
    bookQuery.addBindValue(indexedAt);
    bookQuery.addBindValue(loadError.isEmpty() ? 1 : 0);
    bookQuery.addBindValue(databaseText(loadError));
    if (!bookQuery.exec()) {
        if (errorMessage) {
            *errorMessage = bookQuery.lastError().text();
        }
        database.rollback();
        return false;
    }

    QVector<SearchChunk> rows = chunks;
    if (rows.isEmpty() && loadError.isEmpty()) {
        rows.append(SearchChunk{});
    }

    QSqlQuery chunkQuery(database);
    chunkQuery.prepare(QStringLiteral(
        "INSERT INTO content_chunks(source_url, text_start, page, progress, body, "
        "title_terms, author_terms) VALUES(?, ?, ?, ?, ?, ?, ?)"));
    QSqlQuery ftsQuery(database);
    if (ftsAvailable) {
        ftsQuery.prepare(QStringLiteral(
            "INSERT INTO content_fts(source_url, text_start, page, progress, body, "
            "title_terms, author_terms) VALUES(?, ?, ?, ?, ?, ?, ?)"));
    }

    for (qsizetype index = 0; index < rows.size(); ++index) {
        const SearchChunk &chunk = rows.at(index);
        const QString titleTerms = index == 0 ? databaseText(book.title)
                                              : QStringLiteral("");
        const QString authorTerms = index == 0 ? databaseText(book.author)
                                               : QStringLiteral("");
        const QVariantList values = {key,
                                     chunk.start,
                                     chunk.page,
                                     chunk.progress,
                                     databaseText(chunk.body),
                                     titleTerms,
                                     authorTerms};
        for (qsizetype valueIndex = 0; valueIndex < values.size(); ++valueIndex) {
            chunkQuery.bindValue(valueIndex, values.at(valueIndex));
        }
        if (!chunkQuery.exec()) {
            if (errorMessage) {
                *errorMessage = chunkQuery.lastError().text();
            }
            database.rollback();
            return false;
        }
        chunkQuery.finish();

        if (ftsAvailable) {
            for (qsizetype valueIndex = 0; valueIndex < values.size(); ++valueIndex) {
                ftsQuery.bindValue(valueIndex, values.at(valueIndex));
            }
            if (!ftsQuery.exec()) {
                if (errorMessage) {
                    *errorMessage = ftsQuery.lastError().text();
                }
                database.rollback();
                return false;
            }
            ftsQuery.finish();
        }
    }

    if (!database.commit()) {
        if (errorMessage) {
            *errorMessage = database.lastError().text();
        }
        return false;
    }
    return true;
}

bool isBookCurrent(QSqlDatabase &database,
                   const LibraryBook &book,
                   const QFileInfo &fileInfo)
{
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
        "SELECT file_size, modified_msecs, title, author, format_name "
        "FROM indexed_books WHERE source_url = ?"));
    query.addBindValue(sourceKey(book.sourceUrl));
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toLongLong() == fileInfo.size()
           && query.value(1).toLongLong() == fileInfo.lastModified().toMSecsSinceEpoch()
           && query.value(2).toString() == book.title
           && query.value(3).toString() == book.author
           && query.value(4).toString() == book.formatName;
}

LibrarySearchIndexSummary readSummary(QSqlDatabase &database, int totalBooks)
{
    LibrarySearchIndexSummary summary;
    summary.totalBooks = totalBooks;
    QSqlQuery query(database);
    if (query.exec(QStringLiteral(
            "SELECT COALESCE(SUM(success), 0), "
            "COALESCE(SUM(CASE WHEN success = 0 THEN 1 ELSE 0 END), 0), "
            "MAX(indexed_at) FROM indexed_books"))
        && query.next()) {
        summary.indexedBooks = query.value(0).toInt();
        summary.failedBooks = query.value(1).toInt();
        summary.indexedAt = QDateTime::fromString(query.value(2).toString(),
                                                 Qt::ISODateWithMs);
    }
    return summary;
}

} // namespace

LibrarySearchIndex::LibrarySearchIndex(QString databaseFilePath)
    : m_databaseFilePath(std::move(databaseFilePath))
{
}

QString LibrarySearchIndex::databasePathForProfile(const QString &profileDatabaseFilePath)
{
    const QFileInfo profileInfo(profileDatabaseFilePath);
    return profileInfo.dir().filePath(QStringLiteral("content-search.sqlite"));
}

LibrarySearchIndexSummary LibrarySearchIndex::synchronize(const QVector<LibraryBook> &books,
                                                          bool rebuild,
                                                          std::atomic_bool *cancellation,
                                                          const ProgressCallback &progress) const
{
    LibrarySearchIndexSummary summary;
    summary.totalBooks = books.size();
    const auto cancellationRequested = [cancellation]() {
        return cancellation && cancellation->load(std::memory_order_relaxed);
    };
    const auto reportProgress = [&summary, &progress]() {
        if (progress) {
            progress(summary.processedBooks, summary.totalBooks);
        }
    };
    reportProgress();
    SearchDatabase connection(m_databaseFilePath);
    if (!connection.isValid()) {
        summary.errorMessage = connection.errorMessage();
        return summary;
    }

    QSqlDatabase &database = connection.database();
    if (rebuild) {
        if (!database.transaction()) {
            summary.errorMessage = database.lastError().text();
            return summary;
        }
        QSqlQuery clearQuery(database);
        const bool clearedFts = !connection.ftsAvailable()
                                || clearQuery.exec(QStringLiteral("DELETE FROM content_fts"));
        const bool clearedChunks = clearQuery.exec(QStringLiteral("DELETE FROM content_chunks"));
        const bool clearedBooks = clearQuery.exec(QStringLiteral("DELETE FROM indexed_books"));
        if (!clearedFts || !clearedChunks || !clearedBooks || !database.commit()) {
            summary.errorMessage = clearQuery.lastError().text();
            database.rollback();
            return summary;
        }
    }

    QSet<QString> currentSources;
    for (const LibraryBook &book : books) {
        currentSources.insert(sourceKey(book.sourceUrl));
    }

    QVector<QString> staleSources;
    {
        QSqlQuery sourceQuery(database);
        if (!sourceQuery.exec(QStringLiteral("SELECT source_url FROM indexed_books"))) {
            summary.errorMessage = sourceQuery.lastError().text();
            return summary;
        }
        while (sourceQuery.next()) {
            const QString key = sourceQuery.value(0).toString();
            if (!currentSources.contains(key)) {
                staleSources.append(key);
            }
        }
    }

    for (const QString &key : staleSources) {
        if (cancellationRequested()) {
            summary.canceled = true;
            break;
        }
        if (!database.transaction()) {
            summary.errorMessage = database.lastError().text();
            return summary;
        }
        if (!clearBookRows(database,
                           key,
                           connection.ftsAvailable(),
                           &summary.errorMessage)) {
            database.rollback();
            return summary;
        }
        QSqlQuery deleteBook(database);
        deleteBook.prepare(QStringLiteral("DELETE FROM indexed_books WHERE source_url = ?"));
        deleteBook.addBindValue(key);
        if (!deleteBook.exec() || !database.commit()) {
            summary.errorMessage = deleteBook.lastError().text();
            database.rollback();
            return summary;
        }
    }

    if (summary.canceled) {
        LibrarySearchIndexSummary current = readSummary(database, books.size());
        current.processedBooks = summary.processedBooks;
        current.canceled = true;
        return current;
    }

    for (const LibraryBook &book : books) {
        if (cancellationRequested()) {
            summary.canceled = true;
            break;
        }
        if (book.sourceUrl.isLocalFile() && book.fileAvailable && !book.cloudPlaceholder) {
            const QFileInfo fileInfo(localPath(book));
            if (fileInfo.exists() && fileInfo.isFile()
                && !isBookCurrent(database, book, fileInfo)) {
                QString loadError;
                const QVector<SearchChunk> chunks = loadChunks(book, &loadError);
                if (cancellationRequested()) {
                    summary.canceled = true;
                    break;
                }
                if (!storeBook(database,
                               book,
                               fileInfo,
                               chunks,
                               loadError,
                               connection.ftsAvailable(),
                               &summary.errorMessage)) {
                    return summary;
                }
            }
        }
        ++summary.processedBooks;
        reportProgress();
    }

    LibrarySearchIndexSummary current = readSummary(database, books.size());
    current.processedBooks = summary.processedBooks;
    current.canceled = summary.canceled;
    return current;
}

LibrarySearchIndexSummary LibrarySearchIndex::status(int totalBooks) const
{
    SearchDatabase connection(m_databaseFilePath);
    if (!connection.isValid()) {
        LibrarySearchIndexSummary summary;
        summary.totalBooks = totalBooks;
        summary.errorMessage = connection.errorMessage();
        return summary;
    }
    return readSummary(connection.database(), totalBooks);
}

QVector<LibrarySearchHit> LibrarySearchIndex::search(const QString &searchQuery,
                                                     int limit,
                                                     QString *errorMessage) const
{
    const QVector<QString> tokens = queryTokens(searchQuery);
    if (tokens.isEmpty() || limit <= 0) {
        return {};
    }

    SearchDatabase connection(m_databaseFilePath);
    if (!connection.isValid()) {
        if (errorMessage) {
            *errorMessage = connection.errorMessage();
        }
        return {};
    }

    QSqlQuery query(connection.database());
    if (connection.ftsAvailable()) {
        QStringList ftsTerms;
        ftsTerms.reserve(tokens.size());
        for (const QString &token : tokens) {
            ftsTerms.append(token + QLatin1Char('*'));
        }
        query.prepare(QStringLiteral(
            "SELECT content_fts.source_url, indexed_books.title, indexed_books.author, "
            "indexed_books.format_name, content_fts.text_start, content_fts.page, "
            "content_fts.progress, snippet(content_fts, 4, '', '', ' ... ', 32) "
            "FROM content_fts JOIN indexed_books "
            "ON indexed_books.source_url = content_fts.source_url "
            "WHERE indexed_books.success = 1 AND content_fts MATCH ? "
            "ORDER BY bm25(content_fts, 0.0, 0.0, 0.0, 0.0, 1.0, 2.0, 1.5) "
            "LIMIT ?"));
        query.addBindValue(ftsTerms.join(QStringLiteral(" AND ")));
        query.addBindValue(limit);
    } else {
        QStringList tokenConditions;
        for (qsizetype index = 0; index < tokens.size(); ++index) {
            tokenConditions.append(QStringLiteral(
                "(LOWER(content_chunks.body) LIKE ? OR "
                "LOWER(content_chunks.title_terms) LIKE ? OR "
                "LOWER(content_chunks.author_terms) LIKE ?)"));
        }
        query.prepare(QStringLiteral(
                          "SELECT content_chunks.source_url, indexed_books.title, "
                          "indexed_books.author, indexed_books.format_name, "
                          "content_chunks.text_start, content_chunks.page, "
                          "content_chunks.progress, content_chunks.body "
                          "FROM content_chunks JOIN indexed_books "
                          "ON indexed_books.source_url = content_chunks.source_url "
                          "WHERE indexed_books.success = 1 AND ")
                      + tokenConditions.join(QStringLiteral(" AND "))
                      + QStringLiteral(" ORDER BY indexed_books.title LIMIT ?"));
        for (const QString &token : tokens) {
            const QString pattern = QStringLiteral("%%1%").arg(token);
            query.addBindValue(pattern);
            query.addBindValue(pattern);
            query.addBindValue(pattern);
        }
        query.addBindValue(limit);
    }

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return {};
    }

    QVector<LibrarySearchHit> hits;
    while (query.next()) {
        QString snippet = query.value(7).toString();
        if (!connection.ftsAvailable()) {
            snippet = fallbackSnippet(snippet, tokens);
        } else {
            snippet = normalizedSnippet(snippet);
        }
        hits.append({QUrl(query.value(0).toString()),
                     query.value(1).toString(),
                     query.value(2).toString(),
                     query.value(3).toString(),
                     snippet,
                     query.value(4).toInt(),
                     query.value(5).toInt(),
                     query.value(6).toReal()});
    }
    return hits;
}
