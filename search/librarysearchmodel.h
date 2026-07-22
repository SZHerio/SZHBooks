#pragma once

#include "librarysearchindex.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QVariantMap>
#include <QVector>

class LibraryRepository;

class LibrarySearchModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool indexing READ indexing NOTIFY indexingChanged)
    Q_PROPERTY(int indexedBooks READ indexedBooks NOTIFY indexStatusChanged)
    Q_PROPERTY(int totalBooks READ totalBooks NOTIFY indexStatusChanged)
    Q_PROPERTY(int failedBooks READ failedBooks NOTIFY indexStatusChanged)
    Q_PROPERTY(QDateTime indexedAt READ indexedAt NOTIFY indexStatusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    enum Role {
        SourceUrlRole = Qt::UserRole + 1,
        BookTitleRole,
        BookAuthorRole,
        FormatNameRole,
        SnippetRole,
        StartRole,
        PageRole,
        ProgressRole
    };
    Q_ENUM(Role)

    explicit LibrarySearchModel(LibraryRepository *repository,
                                QString databaseFilePath,
                                QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString query() const;
    bool indexing() const;
    int indexedBooks() const;
    int totalBooks() const;
    int failedBooks() const;
    QDateTime indexedAt() const;
    QString errorMessage() const;

    void setQuery(const QString &query);

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void ensureIndexed();
    Q_INVOKABLE void rebuildIndex();
    Q_INVOKABLE void clearError();

signals:
    void queryChanged();
    void countChanged();
    void indexingChanged();
    void indexStatusChanged();
    void errorMessageChanged();

private:
    static QVariantMap toVariantMap(const LibrarySearchHit &hit);
    void markIndexStale();
    void startIndexing(bool rebuild);
    void applyIndexSummary(const LibrarySearchIndexSummary &summary);
    void refreshResults();
    void setErrorMessage(const QString &errorMessage);

    LibraryRepository *m_repository = nullptr;
    QString m_databaseFilePath;
    QVector<LibrarySearchHit> m_hits;
    QFutureWatcher<LibrarySearchIndexSummary> m_indexWatcher;
    QString m_query;
    QString m_errorMessage;
    QDateTime m_indexedAt;
    int m_indexedBooks = 0;
    int m_totalBooks = 0;
    int m_failedBooks = 0;
    bool m_indexing = false;
    bool m_indexStale = true;
    bool m_refreshPending = false;
    bool m_rebuildPending = false;
};
