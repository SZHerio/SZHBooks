#pragma once

#include "../library/librarybook.h"

#include <QDateTime>
#include <QString>
#include <QUrl>
#include <QVector>

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
    QDateTime indexedAt;
    QString errorMessage;
};

class LibrarySearchIndex final
{
public:
    explicit LibrarySearchIndex(QString databaseFilePath);

    static QString databasePathForProfile(const QString &profileDatabaseFilePath);

    LibrarySearchIndexSummary synchronize(const QVector<LibraryBook> &books,
                                          bool rebuild = false) const;
    LibrarySearchIndexSummary status(int totalBooks) const;
    QVector<LibrarySearchHit> search(const QString &query,
                                     int limit,
                                     QString *errorMessage = nullptr) const;

private:
    QString m_databaseFilePath;
};
