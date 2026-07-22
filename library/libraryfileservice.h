#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QQueue>
#include <QUrl>
#include <QVariantList>

class LibraryRepository;

class LibraryFileService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY rootPathChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY operationStateChanged)
    Q_PROPERTY(bool cancellationRequested READ cancellationRequested NOTIFY operationStateChanged)
    Q_PROPERTY(int operationTotal READ operationTotal NOTIFY operationProgressChanged)
    Q_PROPERTY(int operationCompleted READ operationCompleted NOTIFY operationProgressChanged)
    Q_PROPERTY(int importedCount READ importedCount NOTIFY operationProgressChanged)
    Q_PROPERTY(int duplicateCount READ duplicateCount NOTIFY operationProgressChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY operationProgressChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY operationProgressChanged)
    Q_PROPERTY(QString currentFileName READ currentFileName NOTIFY operationProgressChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QUrl lastImportedUrl READ lastImportedUrl NOTIFY lastImportedUrlChanged)

public:
    explicit LibraryFileService(LibraryRepository *repository,
                                QObject *parent = nullptr);
    ~LibraryFileService() override;

    bool available() const;
    bool busy() const;
    bool cancellationRequested() const;
    int operationTotal() const;
    int operationCompleted() const;
    int importedCount() const;
    int duplicateCount() const;
    int failedCount() const;
    qreal progress() const;
    QString currentFileName() const;
    QString errorMessage() const;
    QUrl lastImportedUrl() const;

    void setRootPath(const QString &rootPath);

    Q_INVOKABLE bool importBooks(const QVariantList &sourceUrls,
                                 const QString &collectionPath = {});
    Q_INVOKABLE QUrl importBook(const QUrl &sourceUrl,
                               const QString &collectionPath = {});
    Q_INVOKABLE void cancel();
    Q_INVOKABLE QUrl moveBook(const QUrl &sourceUrl,
                              const QString &collectionPath);
    Q_INVOKABLE QUrl renameBook(const QUrl &sourceUrl,
                                const QString &newName);
    Q_INVOKABLE bool removeBook(const QUrl &sourceUrl, bool deleteFile);
    Q_INVOKABLE bool createCollection(const QString &parentPath,
                                      const QString &name);
    Q_INVOKABLE QString renameCollection(const QString &collectionPath,
                                         const QString &newName);
    Q_INVOKABLE bool removeCollection(const QString &collectionPath);
    Q_INVOKABLE bool isManagedBook(const QUrl &sourceUrl) const;
    Q_INVOKABLE QString collectionForBook(const QUrl &sourceUrl) const;
    Q_INVOKABLE QString fileBaseName(const QUrl &sourceUrl) const;
    Q_INVOKABLE void clearError();

signals:
    void rootPathChanged();
    void operationStateChanged();
    void operationProgressChanged();
    void errorMessageChanged();
    void lastImportedUrlChanged();
    void libraryChanged();
    void batchFinished(int importedCount,
                       int duplicateCount,
                       int failedCount,
                       bool canceled);
    void duplicateDetected(const QUrl &sourceUrl, const QUrl &existingUrl);
    void activityRecorded(const QString &event,
                          const QString &severity,
                          const QString &detail);
    void operationFailed(const QString &errorMessage);

private:
    struct ImportTask final
    {
        QUrl sourceUrl;
        QString collectionPath;
    };

    struct ImportResult final
    {
        enum Status {
            Copied,
            AlreadyManaged,
            Duplicate,
            InvalidSource,
            Unsupported,
            UnavailableCollection,
            ReadFailed,
            CopyFailed
        };

        Status status = InvalidSource;
        QUrl sourceUrl;
        QUrl destinationUrl;
        QString collectionPath;
    };

    static ImportResult performImport(const ImportTask &task,
                                      const QString &rootPath,
                                      const QStringList &supportedSuffixes);
    static QString normalizedCollectionPath(const QString &collectionPath);
    static bool validFileName(const QString &name);
    static QString errorForImport(const ImportResult &result);

    QString absoluteCollectionPath(const QString &collectionPath) const;
    bool ensureIdle();
    bool ensureAvailable();
    bool ensureManagedFile(const QUrl &sourceUrl, QString *filePath = nullptr);
    bool applyMovedBook(const QUrl &oldUrl,
                        const QUrl &newUrl,
                        const QString &collectionPath);
    void beginNextImport();
    void handleImportFinished();
    void finishBatch();
    void resetBatchState(int total);
    void setError(const QString &errorMessage);
    void setLastImportedUrl(const QUrl &sourceUrl);

    LibraryRepository *m_repository = nullptr;
    QFutureWatcher<ImportResult> m_importWatcher;
    QQueue<ImportTask> m_importQueue;
    QString m_rootPath;
    QString m_currentFileName;
    QString m_errorMessage;
    QUrl m_lastImportedUrl;
    int m_operationTotal = 0;
    int m_operationCompleted = 0;
    int m_importedCount = 0;
    int m_duplicateCount = 0;
    int m_failedCount = 0;
    bool m_busy = false;
    bool m_cancellationRequested = false;
};
