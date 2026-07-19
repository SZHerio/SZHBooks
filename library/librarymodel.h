#pragma once

#include "librarybook.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QVariantMap>

class LibraryRepository;

class LibraryModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY sortModeChanged)
    Q_PROPERTY(QString formatFilter READ formatFilter WRITE setFormatFilter NOTIFY formatFilterChanged)
    Q_PROPERTY(QString progressFilter READ progressFilter WRITE setProgressFilter NOTIFY progressFilterChanged)
    Q_PROPERTY(QString collectionFilter READ collectionFilter WRITE setCollectionFilter NOTIFY collectionFilterChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(QStringList availableFormats READ availableFormats NOTIFY availableFormatsChanged)
    Q_PROPERTY(bool hasActiveFilters READ hasActiveFilters NOTIFY hasActiveFiltersChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY totalCountChanged)
    Q_PROPERTY(QVariantMap recentBook READ recentBook NOTIFY recentBookChanged)

public:
    enum Role {
        SourceUrlRole = Qt::UserRole + 1,
        SourcePathRole,
        TitleRole,
        AuthorRole,
        FormatNameRole,
        CoverUrlRole,
        ProgressRole,
        LastOpenedRole,
        FileAvailableRole,
        CollectionPathRole
    };
    Q_ENUM(Role)

    explicit LibraryModel(LibraryRepository *repository, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filterText() const;
    QString sortMode() const;
    QString formatFilter() const;
    QString progressFilter() const;
    QString collectionFilter() const;
    QString viewMode() const;
    QStringList availableFormats() const;
    bool hasActiveFilters() const;
    QString errorMessage() const;
    int totalCount() const;
    QVariantMap recentBook() const;

    void setFilterText(const QString &filterText);
    void setSortMode(const QString &sortMode);
    void setFormatFilter(const QString &formatFilter);
    void setProgressFilter(const QString &progressFilter);
    void setCollectionFilter(const QString &collectionFilter);
    void setViewMode(const QString &viewMode);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void removeBook(int row);
    Q_INVOKABLE bool relinkBook(const QUrl &oldSourceUrl, const QUrl &newSourceUrl);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void clearError();

signals:
    void filterTextChanged();
    void sortModeChanged();
    void formatFilterChanged();
    void progressFilterChanged();
    void collectionFilterChanged();
    void viewModeChanged();
    void availableFormatsChanged();
    void hasActiveFiltersChanged();
    void errorMessageChanged();
    void countChanged();
    void totalCountChanged();
    void recentBookChanged();

private:
    static QVariantMap toVariantMap(const LibraryBook &book);
    bool matchesFilters(const LibraryBook &book) const;
    void rebuildVisibleBooks();
    void rebuildAvailableFormats();
    void updateProgress(const QUrl &sourceUrl, qreal progress);
    void setErrorMessage(const QString &errorMessage);

    LibraryRepository *m_repository = nullptr;
    QVector<LibraryBook> m_allBooks;
    QVector<LibraryBook> m_visibleBooks;
    QStringList m_availableFormats;
    QString m_filterText;
    QString m_sortMode = QStringLiteral("recent");
    QString m_formatFilter = QStringLiteral("all");
    QString m_progressFilter = QStringLiteral("all");
    QString m_collectionFilter;
    QString m_viewMode = QStringLiteral("grid");
    QString m_errorMessage;
};
