#pragma once

#include "bookmetadatapatch.h"
#include "customcoverstore.h"
#include "librarybook.h"

#include <QObject>
#include <QVector>

#include <optional>

class BookMetadataService;
class LocalStateStore;

class LibraryRepository final : public QObject
{
    Q_OBJECT

public:
    explicit LibraryRepository(LocalStateStore *store,
                               BookMetadataService *metadataService,
                               QObject *parent = nullptr);

    QVector<LibraryBook> books();
    std::optional<LibraryBook> book(const QUrl &sourceUrl);
    QString sortMode() const;
    QString viewMode() const;

    void setSortMode(const QString &sortMode);
    void setViewMode(const QString &viewMode);
    void recordBookOpened(const QUrl &sourceUrl,
                          const QString &title,
                          const QString &author,
                          const QString &formatName);
    bool registerBook(const QUrl &sourceUrl,
                      const QString &collectionPath,
                      bool restoreExcluded = false,
                      bool inspectMetadata = true);
    bool supports(const QUrl &sourceUrl) const;
    void removeBook(const QUrl &sourceUrl);
    void setBookCollection(const QUrl &sourceUrl, const QString &collectionPath);
    bool relinkBook(const QUrl &oldSourceUrl,
                    const QUrl &newSourceUrl,
                    QString *errorMessage = nullptr);
    bool updateBookDetails(const QVector<QUrl> &sourceUrls,
                           const BookMetadataPatch &patch,
                           QString *errorMessage = nullptr);
    bool setCustomCover(const QUrl &sourceUrl,
                        const QUrl &imageUrl,
                        QString *errorMessage = nullptr);
    void setManagedRootPath(const QString &rootPath);

signals:
    void changed();
    void documentProgressChanged(const QUrl &sourceUrl, qreal progress);

private:
    LocalStateStore *m_store = nullptr;
    BookMetadataService *m_metadataService = nullptr;
    CustomCoverStore m_customCoverStore;
};
