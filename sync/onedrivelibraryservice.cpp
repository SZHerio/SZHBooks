#include "onedrivelibraryservice.h"

#include "portableprofilemapper.h"
#include "../library/cloudfileavailability.h"
#include "../library/librarybook.h"
#include "../library/libraryrepository.h"
#include "../storage/localstatestore.h"

#include <QDir>
#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include <algorithm>
#include <iterator>
#include <utility>

namespace {

const QString metadataDirectoryName = QStringLiteral(".szhbooks");

QString cleanAbsolutePath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool samePath(const QString &left, const QString &right)
{
    return cleanAbsolutePath(left).compare(cleanAbsolutePath(right),
#ifdef Q_OS_WIN
                                           Qt::CaseInsensitive
#else
                                           Qt::CaseSensitive
#endif
                                           ) == 0;
}

QString normalizedFileName(QString fileName)
{
    fileName = fileName.trimmed();
    return fileName.isEmpty() ? QStringLiteral("book") : fileName;
}

} // namespace

OneDriveLibraryService::OneDriveLibraryService(LocalStateStore *store,
                                               LibraryRepository *repository,
                                               QObject *parent)
    : OneDriveLibraryService(store, repository, QString(), parent)
{
}

OneDriveLibraryService::OneDriveLibraryService(LocalStateStore *store,
                                               LibraryRepository *repository,
                                               const QString &configurationFilePath,
                                               QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_repository(repository)
    , m_configuration(configurationFilePath)
    , m_profileSync(store)
    , m_fileService(repository)
    , m_activityModel(m_configuration.settingsFilePath())
    , m_rootPath(m_configuration.rootPath())
    , m_suggestedRootPath(suggestedRootPath(&m_oneDriveDetected))
    , m_lastSyncedAt(m_configuration.lastSyncedAt())
{
    Q_ASSERT(m_store);
    Q_ASSERT(m_repository);

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(650);
    m_profileTimer.setSingleShot(true);
    m_profileTimer.setInterval(1200);
    m_retryTimer.setSingleShot(true);

    connect(&m_refreshTimer,
            &QTimer::timeout,
            this,
            &OneDriveLibraryService::synchronizeNow);
    connect(&m_profileTimer,
            &QTimer::timeout,
            this,
            &OneDriveLibraryService::synchronizeNow);
    connect(&m_retryTimer,
            &QTimer::timeout,
            this,
            [this]() {
                m_nextRetryAt = {};
                emit retryStateChanged();
                synchronizeNow();
            });
    connect(&m_watcher,
            &QFileSystemWatcher::directoryChanged,
            this,
            [this]() { m_refreshTimer.start(); });
    connect(&m_watcher,
            &QFileSystemWatcher::fileChanged,
            this,
            [this]() { m_refreshTimer.start(); });
    connect(m_store,
            &LocalStateStore::profileChanged,
            this,
            &OneDriveLibraryService::scheduleSynchronization);
    connect(&m_fileService,
            &LibraryFileService::libraryChanged,
            this,
            [this]() {
                scanLibrary();
                scheduleSynchronization();
            });
    connect(&m_fileService,
            &LibraryFileService::activityRecorded,
            this,
            [this](const QString &event,
                   const QString &severity,
                   const QString &detail) {
                m_activityModel.append(event, severity, detail);
            });
    connect(&m_conflictModel,
            &SyncConflictModel::countChanged,
            this,
            &OneDriveLibraryService::conflictCountChanged);

    m_available = !m_rootPath.isEmpty()
                  && QFileInfo(m_rootPath).isDir();
    m_fileService.setRootPath(m_rootPath);
    if (configured()) {
        m_status = m_available ? QStringLiteral("pending")
                               : QStringLiteral("unavailable");
    }
    if (configured()) {
        refreshConflicts();
    }
    if (m_available) {
        QTimer::singleShot(0, this, &OneDriveLibraryService::synchronizeNow);
    } else if (configured()) {
        scheduleRetry();
    }
}

bool OneDriveLibraryService::configured() const
{
    return !m_rootPath.isEmpty();
}

bool OneDriveLibraryService::available() const
{
    return m_available;
}

QUrl OneDriveLibraryService::rootFolder() const
{
    return m_rootPath.isEmpty() ? QUrl() : QUrl::fromLocalFile(m_rootPath);
}

QString OneDriveLibraryService::rootDisplayPath() const
{
    return QDir::toNativeSeparators(m_rootPath);
}

QUrl OneDriveLibraryService::suggestedFolder() const
{
    return m_suggestedRootPath.isEmpty()
               ? QUrl()
               : QUrl::fromLocalFile(m_suggestedRootPath);
}

QString OneDriveLibraryService::suggestedDisplayPath() const
{
    return QDir::toNativeSeparators(m_suggestedRootPath);
}

bool OneDriveLibraryService::oneDriveDetected() const
{
    return m_oneDriveDetected;
}

QStringList OneDriveLibraryService::collections() const
{
    return m_collections;
}

bool OneDriveLibraryService::syncing() const
{
    return m_syncing;
}

QString OneDriveLibraryService::status() const
{
    return m_status;
}

QString OneDriveLibraryService::errorMessage() const
{
    return m_errorMessage;
}

QDateTime OneDriveLibraryService::lastSyncedAt() const
{
    return m_lastSyncedAt;
}

int OneDriveLibraryService::conflictCount() const
{
    return m_conflictModel.rowCount();
}

SyncActivityModel *OneDriveLibraryService::activityModel()
{
    return &m_activityModel;
}

SyncConflictModel *OneDriveLibraryService::conflictModel()
{
    return &m_conflictModel;
}

bool OneDriveLibraryService::retryScheduled() const
{
    return m_retryTimer.isActive();
}

QDateTime OneDriveLibraryService::nextRetryAt() const
{
    return m_nextRetryAt;
}

QUrl OneDriveLibraryService::conflictsFolder() const
{
    return m_rootPath.isEmpty()
               ? QUrl()
               : QUrl::fromLocalFile(
                     QDir(m_rootPath).filePath(metadataDirectoryName
                                               + QStringLiteral("/conflicts")));
}

int OneDriveLibraryService::cloudPlaceholderCount() const
{
    return m_cloudPlaceholderCount;
}

LibraryFileService *OneDriveLibraryService::fileService()
{
    return &m_fileService;
}

bool OneDriveLibraryService::setRootFolder(const QUrl &folderUrl)
{
    clearError();
    if (!folderUrl.isLocalFile()) {
        setError(tr("Choose a local OneDrive folder."));
        return false;
    }

    const QString rootPath = cleanAbsolutePath(folderUrl.toLocalFile());
    const QFileInfo rootInfo(rootPath);
    if (rootInfo.exists() && !rootInfo.isDir()) {
        setError(tr("The selected path is not a folder."));
        return false;
    }
    const bool rootChanged = m_rootPath.isEmpty() || !samePath(m_rootPath, rootPath);
    if (rootChanged && m_fileService.busy()) {
        setError(tr("Wait for the current library import before changing folders."));
        return false;
    }
    if (!QDir().mkpath(rootPath)) {
        setError(tr("Could not create the selected library folder."));
        return false;
    }

    if (configured() && rootChanged && m_available) {
        synchronizeNow();
    }

    const bool availabilityChangedValue = !m_available;
    m_rootPath = rootPath;
    m_available = true;
    resetRetry();
    m_configuration.setRootPath(rootPath);
    m_fileService.setRootPath(rootPath);
    QDir().mkpath(QDir(rootPath).filePath(metadataDirectoryName));
    if (rootChanged) {
        emit rootFolderChanged();
    }
    if (availabilityChangedValue) {
        emit availabilityChanged();
    }

    m_activityModel.append(QStringLiteral("folderConfigured"),
                           QStringLiteral("info"),
                           rootDisplayPath());
    refreshConflicts();
    migrateExistingBooks();
    return synchronizeNow();
}

bool OneDriveLibraryService::useSuggestedFolder()
{
    return setRootFolder(suggestedFolder());
}

bool OneDriveLibraryService::scanNow()
{
    clearError();
    if (!ensureAvailable()) {
        return false;
    }
    const bool scanned = scanLibrary();
    if (scanned) {
        scheduleSynchronization();
    }
    return scanned;
}

bool OneDriveLibraryService::synchronizeNow()
{
    if (m_syncing) {
        return false;
    }

    clearError();
    if (!ensureAvailable()) {
        return false;
    }

    setSyncing(true);
    setStatus(QStringLiteral("syncing"));
    m_activityModel.append(QStringLiteral("syncStarted"));
    m_profileTimer.stop();
    m_refreshTimer.stop();

    if (!scanLibrary()) {
        setSyncing(false);
        return false;
    }

    m_store->sync();
    const ProfileSyncResult result = m_profileSync.synchronize(
        m_rootPath,
        m_configuration.baseProfile(),
        m_configuration.deviceId());
    if (!result.success) {
        setError(result.errorMessage, QStringLiteral("error"), result.retryable);
        setSyncing(false);
        refreshWatchPaths();
        return false;
    }

    m_configuration.setBaseProfile(result.mergedProfile);
    m_configuration.setLastSyncedAt(result.syncedAt);
    if (m_lastSyncedAt != result.syncedAt) {
        m_lastSyncedAt = result.syncedAt;
        emit lastSyncedAtChanged();
    }
    refreshConflicts();
    if (result.localProfileChanged) {
        emit profileApplied();
        scanLibrary();
    }
    if (!result.conflictKeys.isEmpty()) {
        setStatus(QStringLiteral("attention"));
        m_activityModel.append(QStringLiteral("conflictsDetected"),
                               QStringLiteral("warning"),
                               QString::number(result.conflictKeys.size()));
        emit conflictsDetected(QUrl::fromLocalFile(result.conflictFilePath),
                               result.conflictKeys.size());
    } else if (conflictCount() > 0) {
        setStatus(QStringLiteral("attention"));
    } else {
        setStatus(QStringLiteral("synced"));
    }

    resetRetry();
    m_activityModel.append(QStringLiteral("syncCompleted"));
    refreshWatchPaths();
    setSyncing(false);
    emit synchronizationCompleted();
    return true;
}

QUrl OneDriveLibraryService::importBook(const QUrl &sourceUrl,
                                        const QString &collectionPath)
{
    const QUrl importedUrl = m_fileService.importBook(sourceUrl, collectionPath);
    if (importedUrl.isEmpty() && !m_fileService.errorMessage().isEmpty()) {
        setError(m_fileService.errorMessage());
    }
    return importedUrl;
}

bool OneDriveLibraryService::createCollection(const QString &parentPath,
                                              const QString &name)
{
    const bool created = m_fileService.createCollection(parentPath, name);
    if (!created && !m_fileService.errorMessage().isEmpty()) {
        setError(m_fileService.errorMessage());
    }
    return created;
}

QString OneDriveLibraryService::collectionForBook(const QUrl &sourceUrl) const
{
    return m_fileService.collectionForBook(sourceUrl);
}

bool OneDriveLibraryService::retryNow()
{
    if (!configured() || m_syncing) {
        return false;
    }
    resetRetry();
    return synchronizeNow();
}

bool OneDriveLibraryService::openRootFolder() const
{
    return available() && QDesktopServices::openUrl(rootFolder());
}

bool OneDriveLibraryService::openConflictsFolder() const
{
    if (!configured()) {
        return false;
    }
    const QString path = QDir(m_rootPath).filePath(
        metadataDirectoryName + QStringLiteral("/conflicts"));
    return QDir().mkpath(path)
           && QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

bool OneDriveLibraryService::resolveConflict(const QString &conflictId,
                                             const QString &resolution)
{
    clearError();
    if (!ensureAvailable()) {
        return false;
    }

    const bool useRemote = resolution == QLatin1String("remote");
    if (!useRemote && resolution != QLatin1String("local")) {
        setError(tr("Choose which version of the conflicting value to keep."));
        return false;
    }

    SyncConflictChoice choice;
    if (!m_conflictModel.choice(conflictId, useRemote, &choice)) {
        setError(tr("This synchronization conflict is no longer available."));
        return false;
    }

    if (useRemote) {
        QVariantMap portable = PortableProfileMapper::toPortable(
            m_store->profileValues(), m_rootPath);
        if (choice.present) {
            portable.insert(choice.key, choice.value);
        } else {
            portable.remove(choice.key);
        }
        const QVariantMap replacement = PortableProfileMapper::applyPortable(
            m_store->profileValues(), portable, m_rootPath);
        QString storageError;
        if (!m_store->replaceProfileValues(replacement, &storageError)) {
            setError(tr("Could not apply the selected conflict version: %1")
                         .arg(storageError));
            return false;
        }
    }

    QString conflictError;
    if (!m_conflictModel.markResolved(conflictId, resolution, &conflictError)) {
        setError(conflictError);
        return false;
    }

    m_activityModel.append(QStringLiteral("conflictResolved"),
                           QStringLiteral("info"),
                           choice.key);
    if (useRemote) {
        emit profileApplied();
    }
    setStatus(conflictCount() > 0 ? QStringLiteral("attention")
                                  : QStringLiteral("pending"));
    scheduleSynchronization();
    return true;
}

int OneDriveLibraryService::resolveAllConflicts(const QString &resolution)
{
    QStringList conflictIds;
    conflictIds.reserve(m_conflictModel.rowCount());
    for (int row = 0; row < m_conflictModel.rowCount(); ++row) {
        conflictIds.append(m_conflictModel.index(row, 0)
                               .data(SyncConflictModel::ConflictIdRole)
                               .toString());
    }

    int resolved = 0;
    for (const QString &conflictId : std::as_const(conflictIds)) {
        if (!resolveConflict(conflictId, resolution)) {
            break;
        }
        ++resolved;
    }
    return resolved;
}

void OneDriveLibraryService::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorMessageChanged();
    if (m_status == QLatin1String("error")
        || m_status == QLatin1String("unavailable")) {
        setStatus(configured() ? QStringLiteral("pending")
                               : QStringLiteral("notConfigured"));
    }
}

void OneDriveLibraryService::scheduleSynchronization()
{
    if (!configured()) {
        return;
    }
    if (m_syncing) {
        return;
    }
    m_profileTimer.start();
}

QString OneDriveLibraryService::suggestedRootPath(bool *detected)
{
    for (const char *variable : {"OneDrive", "OneDriveConsumer", "OneDriveCommercial"}) {
        const QString root = qEnvironmentVariable(variable).trimmed();
        if (!root.isEmpty()) {
            if (detected) {
                *detected = true;
            }
            return QDir(root).filePath(QStringLiteral("SZHBooks"));
        }
    }

    if (detected) {
        *detected = false;
    }
    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty()) {
        documents = QDir::homePath();
    }
    return QDir(documents).filePath(QStringLiteral("SZHBooks"));
}

QString OneDriveLibraryService::normalizedCollectionPath(const QString &collectionPath)
{
    QString path = collectionPath.trimmed();
    path.replace(u'\\', u'/');
    path = QDir::cleanPath(path);
    if (path.isEmpty() || path == QLatin1String(".") || path == QLatin1String("all")) {
        return {};
    }
    if (QDir::isAbsolutePath(path)
        || path == QLatin1String("..")
        || path.startsWith(QStringLiteral("../"))
        || path.section(u'/', 0, 0).compare(metadataDirectoryName,
                                            Qt::CaseInsensitive) == 0) {
        return {};
    }
    return QString(path).replace(u'\\', u'/');
}

bool OneDriveLibraryService::ensureAvailable()
{
    if (!configured()) {
        setError(tr("Choose a OneDrive library folder first."));
        return false;
    }
    const bool availableNow = QFileInfo(m_rootPath).isDir();
    if (m_available != availableNow) {
        m_available = availableNow;
        emit availabilityChanged();
        m_activityModel.append(availableNow
                                   ? QStringLiteral("folderAvailable")
                                   : QStringLiteral("folderUnavailable"),
                               availableNow ? QStringLiteral("info")
                                            : QStringLiteral("warning"),
                               rootDisplayPath());
    }
    if (!availableNow) {
        setError(tr("The OneDrive library folder is unavailable."),
                 QStringLiteral("unavailable"),
                 true);
        return false;
    }
    return true;
}

bool OneDriveLibraryService::scanLibrary()
{
    if (!ensureAvailable()) {
        return false;
    }

    QStringList collections;
    int cloudPlaceholderCount = 0;
    scanDirectory(m_rootPath, QString(), &collections, &cloudPlaceholderCount);
    std::sort(collections.begin(), collections.end(), [](const QString &left,
                                                         const QString &right) {
        return QString::localeAwareCompare(left, right) < 0;
    });
    if (m_collections != collections) {
        m_collections = collections;
        emit collectionsChanged();
    }
    if (m_cloudPlaceholderCount != cloudPlaceholderCount) {
        m_cloudPlaceholderCount = cloudPlaceholderCount;
        emit cloudPlaceholderCountChanged();
    }
    refreshWatchPaths();
    return true;
}

void OneDriveLibraryService::scanDirectory(const QString &absolutePath,
                                           const QString &relativePath,
                                           QStringList *collections,
                                           int *cloudPlaceholderCount)
{
    const QDir directory(absolutePath);
    const QFileInfoList entries = directory.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot,
        QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);
    for (const QFileInfo &entry : entries) {
        if (entry.isSymLink()) {
            continue;
        }
        const QString childRelativePath = relativePath.isEmpty()
                                              ? entry.fileName()
                                              : relativePath + u'/' + entry.fileName();
        if (entry.isDir()) {
            if (entry.fileName().compare(metadataDirectoryName,
                                         Qt::CaseInsensitive) == 0) {
                continue;
            }
            collections->append(childRelativePath);
            scanDirectory(entry.absoluteFilePath(),
                          childRelativePath,
                          collections,
                          cloudPlaceholderCount);
            continue;
        }

        const QUrl sourceUrl = QUrl::fromLocalFile(entry.absoluteFilePath());
        if (m_repository->supports(sourceUrl)) {
            const bool onlineOnly = CloudFileAvailability::isOnlineOnly(
                entry.absoluteFilePath());
            if (onlineOnly) {
                ++(*cloudPlaceholderCount);
            }
            m_repository->registerBook(sourceUrl,
                                       relativePath,
                                       false,
                                       !onlineOnly);
        }
    }
}

QString OneDriveLibraryService::absoluteCollectionPath(const QString &collectionPath) const
{
    const QString normalized = normalizedCollectionPath(collectionPath);
    if (!collectionPath.trimmed().isEmpty()
        && normalized.isEmpty()
        && collectionPath.trimmed() != QLatin1String("all")) {
        return {};
    }

    const QString path = normalized.isEmpty()
                             ? m_rootPath
                             : QDir(m_rootPath).absoluteFilePath(normalized);
    const QString relative = QDir(m_rootPath).relativeFilePath(path);
    if (QDir::isAbsolutePath(relative)
        || relative == QLatin1String("..")
        || relative.startsWith(QStringLiteral("../"))
        || relative.startsWith(QStringLiteral("..\\"))) {
        return {};
    }
    return cleanAbsolutePath(path);
}

QString OneDriveLibraryService::destinationForCopy(const QFileInfo &sourceFile,
                                                   const QString &collectionPath) const
{
    const QString directoryPath = absoluteCollectionPath(collectionPath);
    if (directoryPath.isEmpty()) {
        return {};
    }

    const QString baseName = normalizedFileName(sourceFile.completeBaseName());
    const QString suffix = sourceFile.suffix();
    QString fileName = sourceFile.fileName();
    QString candidate = QDir(directoryPath).filePath(fileName);
    for (int index = 2; QFileInfo::exists(candidate); ++index) {
        fileName = suffix.isEmpty()
                       ? QStringLiteral("%1 (%2)").arg(baseName).arg(index)
                       : QStringLiteral("%1 (%2).%3").arg(baseName).arg(index).arg(suffix);
        candidate = QDir(directoryPath).filePath(fileName);
    }
    return candidate;
}

void OneDriveLibraryService::migrateExistingBooks()
{
    const QVector<LibraryBook> books = m_repository->books();
    QString importedPath;
    for (const LibraryBook &book : books) {
        if (!book.fileAvailable
            || !book.sourceUrl.isLocalFile()
            || PortableProfileMapper::isManagedBook(book.sourceUrl, m_rootPath)) {
            continue;
        }

        if (importedPath.isEmpty()) {
            importedPath = QDir(m_rootPath).filePath(QStringLiteral("Imported"));
            if (!QDir().mkpath(importedPath)) {
                setError(tr("Could not create the Imported collection."));
                return;
            }
        }

        const QFileInfo sourceInfo(book.sourceUrl.toLocalFile());
        const QString destinationPath = destinationForCopy(sourceInfo,
                                                           QStringLiteral("Imported"));
        if (destinationPath.isEmpty()
            || !QFile::copy(sourceInfo.absoluteFilePath(), destinationPath)
            || !m_store->relinkDocument(book.sourceUrl,
                                        QUrl::fromLocalFile(destinationPath))) {
            setError(tr("Some existing books could not be copied into the OneDrive library."));
            continue;
        }
        m_store->setBookCollection(QUrl::fromLocalFile(destinationPath),
                                   QStringLiteral("Imported"));
    }
}

void OneDriveLibraryService::refreshWatchPaths()
{
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }
    if (!m_available) {
        return;
    }

    QStringList directories{m_rootPath};
    for (const QString &collection : m_collections) {
        const QString path = absoluteCollectionPath(collection);
        if (!path.isEmpty() && QFileInfo(path).isDir()) {
            directories.append(path);
        }
    }
    const QString metadataPath = QDir(m_rootPath).filePath(metadataDirectoryName);
    if (QFileInfo(metadataPath).isDir()) {
        directories.append(metadataPath);
    }
    m_watcher.addPaths(directories);

    const QString profilePath = ProfileSyncEngine::profileFilePath(m_rootPath);
    if (QFileInfo::exists(profilePath)) {
        m_watcher.addPath(profilePath);
    }
}

void OneDriveLibraryService::setSyncing(bool syncing)
{
    if (m_syncing == syncing) {
        return;
    }
    m_syncing = syncing;
    emit syncingChanged();
}

void OneDriveLibraryService::setStatus(const QString &status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    emit statusChanged();
}

void OneDriveLibraryService::setError(const QString &errorMessage,
                                      const QString &status,
                                      bool retry)
{
    const QString message = errorMessage.trimmed().isEmpty()
                                ? tr("OneDrive synchronization failed.")
                                : errorMessage;
    const bool changed = m_errorMessage != message || m_status != status;
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
    setStatus(status);
    if (retry) {
        scheduleRetry();
    }
    if (changed) {
        m_activityModel.append(status == QLatin1String("unavailable")
                                   ? QStringLiteral("folderUnavailable")
                                   : QStringLiteral("syncFailed"),
                               QStringLiteral("error"),
                               message);
        emit operationFailed(m_errorMessage);
    }
}

void OneDriveLibraryService::scheduleRetry()
{
    if (!configured() || m_retryTimer.isActive()) {
        return;
    }
    static constexpr int delays[] = {2, 5, 15, 30, 60};
    const int index = qMin(m_retryAttempt,
                           static_cast<int>(std::size(delays)) - 1);
    const int delaySeconds = delays[index];
    ++m_retryAttempt;
    m_nextRetryAt = QDateTime::currentDateTimeUtc().addSecs(delaySeconds);
    m_retryTimer.start(delaySeconds * 1000);
    emit retryStateChanged();
}

void OneDriveLibraryService::resetRetry()
{
    const bool stateChanged = m_retryTimer.isActive() || m_nextRetryAt.isValid();
    m_retryTimer.stop();
    m_retryAttempt = 0;
    m_nextRetryAt = {};
    if (stateChanged) {
        emit retryStateChanged();
    }
}

void OneDriveLibraryService::refreshConflicts()
{
    m_conflictModel.load(m_rootPath);
}
