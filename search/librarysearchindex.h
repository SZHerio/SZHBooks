#pragma once

#include "../library/librarybook.h"

#include <QDateTime>
#include <QString>
#include <QUrl>
#include <QVector>

#include <atomic>
#include <functional>

struct LibrarySearchHit final
{
    QUrl sourceUrl;
    QString bookTitle;
    QString bookAuthor;
    QString formatName;
    QString snippet;
    int start = -1;
    int page = -1;
    qreal progress = 0;
};

struct LibrarySearchIndexSummary final
{
    int totalBooks = 0;
    int indexedBooks = 0;
    int failedBooks = 0;
    int processedBooks = 0;
    bool canceled = false;
    QDateTime indexedAt;
    QString errorMessage;
};

class LibrarySearchIndex final
{
public:
    using ProgressCallback = std::function<void(int completed, int total)>;

    explicit LibrarySearchIndex(QString databaseFilePath);

    static QString databasePathForProfile(const QString &profileDatabaseFilePath);

    LibrarySearchIndexSummary synchronize(const QVector<LibraryBook> &books,
                                          bool rebuild = false,
                                          std::atomic_bool *cancellation = nullptr,
                                          const ProgressCallback &progress = {}) const;
    LibrarySearchIndexSummary status(int totalBooks) const;
    QVector<LibrarySearchHit> search(const QString &query,
                                     int limit,
                                     QString *errorMessage = nullptr) const;

private:
    QString m_databaseFilePath;
};
