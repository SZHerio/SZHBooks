#pragma once

#include "profilesyncengine.h"
#include "syncactivitymodel.h"
#include "syncconfigurationstore.h"
#include "syncconflictmodel.h"
#include "../library/libraryfileservice.h"

#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>

class LibraryRepository;
class LocalStateStore;
class QFileInfo;

class OneDriveLibraryService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool configured READ configured NOTIFY rootFolderChanged)
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(QUrl rootFolder READ rootFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(QString rootDisplayPath READ rootDisplayPath NOTIFY rootFolderChanged)
    Q_PROPERTY(QUrl suggestedFolder READ suggestedFolder CONSTANT)
    Q_PROPERTY(QString suggestedDisplayPath READ suggestedDisplayPath CONSTANT)
    Q_PROPERTY(bool oneDriveDetected READ oneDriveDetected CONSTANT)
    Q_PROPERTY(QStringList collections READ collections NOTIFY collectionsChanged)
    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QDateTime lastSyncedAt READ lastSyncedAt NOTIFY lastSyncedAtChanged)
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY conflictCountChanged)
    Q_PROPERTY(SyncActivityModel *activityModel READ activityModel CONSTANT)
    Q_PROPERTY(SyncConflictModel *conflictModel READ conflictModel CONSTANT)
    Q_PROPERTY(bool retryScheduled READ retryScheduled NOTIFY retryStateChanged)
    Q_PROPERTY(QDateTime nextRetryAt READ nextRetryAt NOTIFY retryStateChanged)
    Q_PROPERTY(QUrl conflictsFolder READ conflictsFolder NOTIFY rootFolderChanged)
    Q_PROPERTY(int cloudPlaceholderCount READ cloudPlaceholderCount NOTIFY cloudPlaceholderCountChanged)
    Q_PROPERTY(LibraryFileService *fileService READ fileService CONSTANT)

public:
    explicit OneDriveLibraryService(LocalStateStore *store,
                                    LibraryRepository *repository,
                                    QObject *parent = nullptr);
    OneDriveLibraryService(LocalStateStore *store,
                           LibraryRepository *repository,
                           const QString &configurationFilePath,
                           QObject *parent = nullptr);

    bool configured() const;
    bool available() const;
    QUrl rootFolder() const;
    QString rootDisplayPath() const;
    QUrl suggestedFolder() const;
    QString suggestedDisplayPath() const;
    bool oneDriveDetected() const;
    QStringList collections() const;
    bool syncing() const;
    QString status() const;
    QString errorMessage() const;
    QDateTime lastSyncedAt() const;
    int conflictCount() const;
    SyncActivityModel *activityModel();
    SyncConflictModel *conflictModel();
    bool retryScheduled() const;
    QDateTime nextRetryAt() const;
    QUrl conflictsFolder() const;
    int cloudPlaceholderCount() const;
    LibraryFileService *fileService();

    Q_INVOKABLE bool setRootFolder(const QUrl &folderUrl);
    Q_INVOKABLE bool useSuggestedFolder();
    Q_INVOKABLE bool scanNow();
    Q_INVOKABLE bool synchronizeNow();
    Q_INVOKABLE QUrl importBook(const QUrl &sourceUrl,
                                const QString &collectionPath = {});
    Q_INVOKABLE bool createCollection(const QString &parentPath,
                                      const QString &name);
    Q_INVOKABLE QString collectionForBook(const QUrl &sourceUrl) const;
    Q_INVOKABLE bool retryNow();
    Q_INVOKABLE bool openRootFolder() const;
    Q_INVOKABLE bool openConflictsFolder() const;
    Q_INVOKABLE bool resolveConflict(const QString &conflictId,
                                     const QString &resolution);
    Q_INVOKABLE int resolveAllConflicts(const QString &resolution);
    Q_INVOKABLE void clearError();

public slots:
    void scheduleSynchronization();

signals:
    void rootFolderChanged();
    void availabilityChanged();
    void collectionsChanged();
    void syncingChanged();
    void statusChanged();
    void errorMessageChanged();
    void lastSyncedAtChanged();
    void conflictCountChanged();
    void retryStateChanged();
    void cloudPlaceholderCountChanged();
    void profileApplied();
    void synchronizationCompleted();
    void conflictsDetected(const QUrl &conflictFile, int count);
    void operationFailed(const QString &errorMessage);

private:
    static QString suggestedRootPath(bool *oneDriveDetected);
    static QString normalizedCollectionPath(const QString &collectionPath);

    bool ensureAvailable();
    bool scanLibrary();
    void scanDirectory(const QString &absolutePath,
                       const QString &relativePath,
                       QStringList *collections,
                       int *cloudPlaceholderCount);
    QString absoluteCollectionPath(const QString &collectionPath) const;
    QString destinationForCopy(const QFileInfo &sourceFile,
                               const QString &collectionPath) const;
    void migrateExistingBooks();
    void refreshWatchPaths();
    void setSyncing(bool syncing);
    void setStatus(const QString &status);
    void setError(const QString &errorMessage,
                  const QString &status = QStringLiteral("error"),
                  bool scheduleRetry = false);
    void scheduleRetry();
    void resetRetry();
    void refreshConflicts();

    LocalStateStore *m_store = nullptr;
    LibraryRepository *m_repository = nullptr;
    SyncConfigurationStore m_configuration;
    ProfileSyncEngine m_profileSync;
    LibraryFileService m_fileService;
    SyncActivityModel m_activityModel;
    SyncConflictModel m_conflictModel;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
    QTimer m_profileTimer;
    QTimer m_retryTimer;
    QString m_rootPath;
    QString m_suggestedRootPath;
    QStringList m_collections;
    QString m_status = QStringLiteral("notConfigured");
    QString m_errorMessage;
    QDateTime m_lastSyncedAt;
    bool m_oneDriveDetected = false;
    bool m_syncing = false;
    bool m_available = false;
    int m_retryAttempt = 0;
    int m_cloudPlaceholderCount = 0;
    QDateTime m_nextRetryAt;
};
