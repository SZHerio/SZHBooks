#include "libraryfileservice.h"

#include "bookmetadataservice.h"
#include "librarybook.h"
#include "libraryrepository.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QtConcurrentRun>

#include <utility>

namespace {

const QString metadataDirectoryName = QStringLiteral(".szhbooks");

QString cleanAbsolutePath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

Qt::CaseSensitivity pathCaseSensitivity()
{
#ifdef Q_OS_WIN
    return Qt::CaseInsensitive;
#else
    return Qt::CaseSensitive;
#endif
}

bool samePath(const QString &left, const QString &right)
{
    return cleanAbsolutePath(left).compare(cleanAbsolutePath(right),
                                           pathCaseSensitivity()) == 0;
}

QString pathIdentity(const QString &path)
{
    QString identity = cleanAbsolutePath(path);
#ifdef Q_OS_WIN
    identity = identity.toLower();
#endif
    return identity;
}

bool pathIsWithin(const QString &path, const QString &rootPath)
{
    if (path.isEmpty() || rootPath.isEmpty()) {
        return false;
    }
    const QString cleanPath = cleanAbsolutePath(path);
    const QString cleanRoot = cleanAbsolutePath(rootPath);
    const QString relative = QDir(cleanRoot).relativeFilePath(cleanPath);
    return cleanPath.compare(cleanRoot, pathCaseSensitivity()) != 0
           && !QDir::isAbsolutePath(relative)
           && relative != QLatin1String("..")
           && !relative.startsWith(QStringLiteral("../"))
           && !relative.startsWith(QStringLiteral("..\\"));
}

QByteArray fileHash(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray buffer(1024 * 1024, Qt::Uninitialized);
    while (!file.atEnd()) {
        const qint64 read = file.read(buffer.data(), buffer.size());
        if (read < 0) {
            return {};
        }
        hash.addData(buffer.constData(), read);
    }
    return hash.result();
}

QString duplicatePath(const QString &sourcePath,
                      const QString &rootPath,
                      const QStringList &supportedSuffixes,
                      bool *sourceReadable)
{
    const QFileInfo sourceInfo(sourcePath);
    QByteArray sourceHash;
    bool hashLoaded = false;
    QDirIterator iterator(rootPath,
                          QDir::Files | QDir::NoSymLinks,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        const QString candidatePath = iterator.next();
        const QFileInfo candidateInfo(candidatePath);
        if (samePath(candidatePath, sourcePath)
            || !supportedSuffixes.contains(candidateInfo.suffix().toLower())
            || candidateInfo.size() != sourceInfo.size()) {
            continue;
        }
        if (!hashLoaded) {
            sourceHash = fileHash(sourcePath);
            hashLoaded = true;
            if (sourceHash.isEmpty() && sourceInfo.size() > 0) {
                if (sourceReadable) {
                    *sourceReadable = false;
                }
                return {};
            }
        }
        if (fileHash(candidatePath) == sourceHash) {
            return candidatePath;
        }
    }
    if (sourceReadable) {
        *sourceReadable = true;
    }
    return {};
}

QString uniqueDestination(const QString &sourcePath, const QString &directoryPath)
{
    const QFileInfo sourceInfo(sourcePath);
    const QString baseName = sourceInfo.completeBaseName().trimmed().isEmpty()
                                 ? QStringLiteral("book")
                                 : sourceInfo.completeBaseName().trimmed();
    const QString suffix = sourceInfo.suffix();
    QString candidate = QDir(directoryPath).filePath(sourceInfo.fileName());
    for (int index = 2; QFileInfo::exists(candidate); ++index) {
        const QString fileName = suffix.isEmpty()
                                     ? QStringLiteral("%1 (%2)").arg(baseName).arg(index)
                                     : QStringLiteral("%1 (%2).%3")
                                           .arg(baseName)
                                           .arg(index)
                                           .arg(suffix);
        candidate = QDir(directoryPath).filePath(fileName);
    }
    return candidate;
}

} // namespace

LibraryFileService::LibraryFileService(LibraryRepository *repository, QObject *parent)
    : QObject(parent)
    , m_repository(repository)
{
    Q_ASSERT(m_repository);
    connect(&m_importWatcher,
            &QFutureWatcher<ImportResult>::finished,
            this,
            &LibraryFileService::handleImportFinished);
}

LibraryFileService::~LibraryFileService()
{
    m_importQueue.clear();
    m_cancellationRequested = true;
    if (m_importWatcher.isRunning()) {
        m_importWatcher.waitForFinished();
    }
}

bool LibraryFileService::available() const
{
    return !m_rootPath.isEmpty() && QFileInfo(m_rootPath).isDir();
}

bool LibraryFileService::busy() const { return m_busy; }
bool LibraryFileService::cancellationRequested() const { return m_cancellationRequested; }
int LibraryFileService::operationTotal() const { return m_operationTotal; }
int LibraryFileService::operationCompleted() const { return m_operationCompleted; }
int LibraryFileService::importedCount() const { return m_importedCount; }
int LibraryFileService::duplicateCount() const { return m_duplicateCount; }
int LibraryFileService::failedCount() const { return m_failedCount; }
QString LibraryFileService::currentFileName() const { return m_currentFileName; }
QString LibraryFileService::errorMessage() const { return m_errorMessage; }
QUrl LibraryFileService::lastImportedUrl() const { return m_lastImportedUrl; }

qreal LibraryFileService::progress() const
{
    return m_operationTotal > 0
               ? qreal(m_operationCompleted) / qreal(m_operationTotal)
               : 0;
}

void LibraryFileService::setRootPath(const QString &rootPath)
{
    const QString cleanPath = rootPath.isEmpty() ? QString() : cleanAbsolutePath(rootPath);
    if (samePath(m_rootPath, cleanPath)) {
        return;
    }
    if (m_busy) {
        setError(tr("Wait for the current import before changing the library folder."));
        return;
    }
    m_rootPath = cleanPath;
    m_repository->setManagedRootPath(m_rootPath);
    emit rootPathChanged();
}

bool LibraryFileService::importBooks(const QVariantList &sourceUrls,
                                     const QString &collectionPath)
{
    clearError();
    if (!ensureIdle()) {
        return false;
    }
    if (!ensureAvailable()) {
        return false;
    }
    const QString normalizedCollection = normalizedCollectionPath(collectionPath);
    if (absoluteCollectionPath(collectionPath).isEmpty()) {
        setError(tr("The selected destination folder is unavailable."));
        return false;
    }

    QQueue<ImportTask> tasks;
    QSet<QString> uniqueSources;
    for (const QVariant &value : sourceUrls) {
        const QUrl sourceUrl = value.toUrl();
        if (!sourceUrl.isLocalFile()) {
            continue;
        }
        const QString sourcePath = cleanAbsolutePath(sourceUrl.toLocalFile());
        const QString identity = pathIdentity(sourcePath);
        if (!uniqueSources.contains(identity)) {
            uniqueSources.insert(identity);
            tasks.enqueue({QUrl::fromLocalFile(sourcePath), normalizedCollection});
        }
    }
    if (tasks.isEmpty()) {
        setError(tr("Choose one or more local book files."));
        return false;
    }

    m_importQueue = tasks;
    resetBatchState(tasks.size());
    beginNextImport();
    return true;
}

QUrl LibraryFileService::importBook(const QUrl &sourceUrl,
                                    const QString &collectionPath)
{
    clearError();
    if (!ensureIdle() || !ensureAvailable()) {
        return {};
    }
    if (absoluteCollectionPath(collectionPath).isEmpty()) {
        setError(tr("The selected destination folder is unavailable."));
        return {};
    }
    const ImportResult result = performImport(
        {sourceUrl, normalizedCollectionPath(collectionPath)},
        m_rootPath,
        BookMetadataService::supportedSuffixes());
    if (result.status != ImportResult::Copied
        && result.status != ImportResult::AlreadyManaged
        && result.status != ImportResult::Duplicate) {
        setError(errorForImport(result));
        return {};
    }

    const QString actualCollection = collectionForBook(result.destinationUrl);
    m_repository->registerBook(result.destinationUrl, actualCollection, true, false);
    setLastImportedUrl(result.destinationUrl);
    if (result.status == ImportResult::Duplicate) {
        emit duplicateDetected(result.sourceUrl, result.destinationUrl);
    }
    emit activityRecorded(QStringLiteral("booksImported"),
                          QStringLiteral("info"),
                          QFileInfo(result.destinationUrl.toLocalFile()).fileName());
    emit libraryChanged();
    return result.destinationUrl;
}

void LibraryFileService::cancel()
{
    if (!m_busy || m_cancellationRequested) {
        return;
    }
    m_cancellationRequested = true;
    m_importQueue.clear();
    emit operationStateChanged();
    if (!m_importWatcher.isRunning()) {
        finishBatch();
    }
}

QUrl LibraryFileService::moveBook(const QUrl &sourceUrl,
                                  const QString &collectionPath)
{
    clearError();
    QString sourcePath;
    if (!ensureIdle() || !ensureAvailable() || !ensureManagedFile(sourceUrl, &sourcePath)) {
        return {};
    }
    const QString normalizedCollection = normalizedCollectionPath(collectionPath);
    const QString destinationDirectory = absoluteCollectionPath(collectionPath);
    if (destinationDirectory.isEmpty()) {
        setError(tr("The selected destination folder is unavailable."));
        return {};
    }
    const QString destinationPath = QDir(destinationDirectory).filePath(
        QFileInfo(sourcePath).fileName());
    if (samePath(sourcePath, destinationPath)) {
        return sourceUrl;
    }
    if (QFileInfo::exists(destinationPath)) {
        setError(tr("A book with this filename already exists in the destination folder."));
        return {};
    }
    if (!QFile::rename(sourcePath, destinationPath)) {
        setError(tr("Could not move the book to the selected folder."));
        return {};
    }

    const QUrl destinationUrl = QUrl::fromLocalFile(destinationPath);
    if (!applyMovedBook(sourceUrl, destinationUrl, normalizedCollection)) {
        QFile::rename(destinationPath, sourcePath);
        return {};
    }
    emit activityRecorded(QStringLiteral("bookMoved"),
                          QStringLiteral("info"),
                          QFileInfo(destinationPath).fileName());
    emit libraryChanged();
    return destinationUrl;
}

QUrl LibraryFileService::renameBook(const QUrl &sourceUrl, const QString &newName)
{
    clearError();
    QString sourcePath;
    if (!ensureIdle() || !ensureAvailable() || !ensureManagedFile(sourceUrl, &sourcePath)) {
        return {};
    }
    QString cleanName = newName.trimmed();
    const QFileInfo sourceInfo(sourcePath);
    const QString suffix = sourceInfo.suffix();
    if (!suffix.isEmpty() && cleanName.endsWith(u'.' + suffix, Qt::CaseInsensitive)) {
        cleanName.chop(suffix.size() + 1);
    }
    if (!validFileName(cleanName)) {
        setError(tr("Use a filename without reserved characters."));
        return {};
    }
    const QString fileName = suffix.isEmpty() ? cleanName : cleanName + u'.' + suffix;
    const QString destinationPath = QDir(sourceInfo.absolutePath()).filePath(fileName);
    if (samePath(sourcePath, destinationPath)) {
        return sourceUrl;
    }
    if (QFileInfo::exists(destinationPath)) {
        setError(tr("A book with this filename already exists."));
        return {};
    }
    if (!QFile::rename(sourcePath, destinationPath)) {
        setError(tr("Could not rename the book file."));
        return {};
    }

    const QUrl destinationUrl = QUrl::fromLocalFile(destinationPath);
    if (!applyMovedBook(sourceUrl,
                        destinationUrl,
                        collectionForBook(destinationUrl))) {
        QFile::rename(destinationPath, sourcePath);
        return {};
    }
    emit activityRecorded(QStringLiteral("bookRenamed"),
                          QStringLiteral("info"),
                          fileName);
    emit libraryChanged();
    return destinationUrl;
}

bool LibraryFileService::removeBook(const QUrl &sourceUrl, bool deleteFile)
{
    clearError();
    if (!ensureIdle()) {
        return false;
    }
    if (!sourceUrl.isLocalFile()) {
        setError(tr("The selected book file is unavailable."));
        return false;
    }
    if (deleteFile) {
        QString sourcePath;
        if (!ensureAvailable() || !ensureManagedFile(sourceUrl, &sourcePath)) {
            return false;
        }
        if (!QFile::moveToTrash(sourcePath)) {
            setError(tr("Could not move the book file to the Trash."));
            return false;
        }
    }
    m_repository->removeBook(sourceUrl);
    emit activityRecorded(QStringLiteral("bookRemoved"),
                          QStringLiteral("info"),
                          QFileInfo(sourceUrl.toLocalFile()).fileName());
    emit libraryChanged();
    return true;
}

bool LibraryFileService::createCollection(const QString &parentPath,
                                          const QString &name)
{
    clearError();
    if (!ensureIdle() || !ensureAvailable()) {
        return false;
    }
    const QString parent = normalizedCollectionPath(parentPath);
    const QString parentDirectory = absoluteCollectionPath(parentPath);
    const QString cleanName = name.trimmed();
    if (parentDirectory.isEmpty() || !QFileInfo(parentDirectory).isDir()) {
        setError(tr("The parent folder is unavailable."));
        return false;
    }
    if (!validFileName(cleanName)) {
        setError(tr("Use a folder name without reserved characters."));
        return false;
    }
    const QString collection = parent.isEmpty() ? cleanName : parent + u'/' + cleanName;
    const QString path = absoluteCollectionPath(collection);
    if (path.isEmpty() || QFileInfo::exists(path)) {
        setError(tr("A folder with this name already exists."));
        return false;
    }
    if (!QDir().mkpath(path)) {
        setError(tr("Could not create the library folder."));
        return false;
    }
    emit activityRecorded(QStringLiteral("collectionCreated"),
                          QStringLiteral("info"),
                          collection);
    emit libraryChanged();
    return true;
}

QString LibraryFileService::renameCollection(const QString &collectionPath,
                                             const QString &newName)
{
    clearError();
    if (!ensureIdle() || !ensureAvailable()) {
        return {};
    }
    const QString collection = normalizedCollectionPath(collectionPath);
    const QString cleanName = newName.trimmed();
    if (collection.isEmpty()) {
        setError(tr("The library root cannot be renamed."));
        return {};
    }
    if (!validFileName(cleanName)) {
        setError(tr("Use a folder name without reserved characters."));
        return {};
    }
    const QString parent = collection.section(u'/', 0, -2);
    const QString destinationCollection = parent.isEmpty()
                                              ? cleanName
                                              : parent + u'/' + cleanName;
    const QString sourcePath = absoluteCollectionPath(collection);
    const QString destinationPath = absoluteCollectionPath(destinationCollection);
    if (sourcePath.isEmpty() || !QFileInfo(sourcePath).isDir()) {
        setError(tr("The selected folder is unavailable."));
        return {};
    }
    if (destinationPath.isEmpty() || QFileInfo::exists(destinationPath)) {
        setError(tr("A folder with this name already exists."));
        return {};
    }

    struct Relink final { QUrl oldUrl; QUrl newUrl; QString collection; };
    QVector<Relink> relinks;
    for (const LibraryBook &book : m_repository->books()) {
        if (!book.fileAvailable || !book.sourceUrl.isLocalFile()
            || !pathIsWithin(book.sourceUrl.toLocalFile(), sourcePath)) {
            continue;
        }
        const QString relative = QDir(sourcePath).relativeFilePath(
            book.sourceUrl.toLocalFile());
        const QUrl newUrl = QUrl::fromLocalFile(
            QDir(destinationPath).filePath(relative));
        QString newCollection = destinationCollection;
        const QString relativeCollection = book.collectionPath.mid(collection.size());
        if (!relativeCollection.isEmpty()) {
            newCollection += relativeCollection;
        }
        relinks.append({book.sourceUrl, newUrl, newCollection});
    }

    if (!QDir().rename(sourcePath, destinationPath)) {
        setError(tr("Could not rename the library folder."));
        return {};
    }
    int completed = 0;
    for (const Relink &relink : std::as_const(relinks)) {
        if (!applyMovedBook(relink.oldUrl, relink.newUrl, relink.collection)) {
            QDir().rename(destinationPath, sourcePath);
            for (int index = completed - 1; index >= 0; --index) {
                m_repository->relinkBook(relinks.at(index).newUrl,
                                         relinks.at(index).oldUrl);
                m_repository->setBookCollection(relinks.at(index).oldUrl,
                                                collection
                                                    + relinks.at(index).collection.mid(
                                                        destinationCollection.size()));
            }
            return {};
        }
        ++completed;
    }

    emit activityRecorded(QStringLiteral("collectionRenamed"),
                          QStringLiteral("info"),
                          collection + QStringLiteral(" -> ") + destinationCollection);
    emit libraryChanged();
    return destinationCollection;
}

bool LibraryFileService::removeCollection(const QString &collectionPath)
{
    clearError();
    if (!ensureIdle() || !ensureAvailable()) {
        return false;
    }
    const QString collection = normalizedCollectionPath(collectionPath);
    const QString path = absoluteCollectionPath(collection);
    if (collection.isEmpty() || path.isEmpty() || !QFileInfo(path).isDir()) {
        setError(tr("Choose an existing library folder."));
        return false;
    }
    const QDir directory(path);
    if (!directory.entryList(QDir::AllEntries | QDir::Hidden | QDir::System
                             | QDir::NoDotAndDotDot).isEmpty()) {
        setError(tr("Only an empty library folder can be removed."));
        return false;
    }
    if (!QDir().rmdir(path)) {
        setError(tr("Could not remove the library folder."));
        return false;
    }
    emit activityRecorded(QStringLiteral("collectionRemoved"),
                          QStringLiteral("info"),
                          collection);
    emit libraryChanged();
    return true;
}

bool LibraryFileService::isManagedBook(const QUrl &sourceUrl) const
{
    return sourceUrl.isLocalFile()
           && pathIsWithin(sourceUrl.toLocalFile(), m_rootPath);
}

QString LibraryFileService::collectionForBook(const QUrl &sourceUrl) const
{
    if (!isManagedBook(sourceUrl)) {
        return {};
    }
    QString relative = QDir(m_rootPath).relativeFilePath(sourceUrl.toLocalFile());
    relative.replace(u'\\', u'/');
    const qsizetype separator = relative.lastIndexOf(u'/');
    return separator < 0 ? QString() : relative.left(separator);
}

QString LibraryFileService::fileBaseName(const QUrl &sourceUrl) const
{
    return sourceUrl.isLocalFile()
               ? QFileInfo(sourceUrl.toLocalFile()).completeBaseName()
               : QString();
}

void LibraryFileService::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorMessageChanged();
}

LibraryFileService::ImportResult LibraryFileService::performImport(
    const ImportTask &task,
    const QString &rootPath,
    const QStringList &supportedSuffixes)
{
    ImportResult result;
    result.sourceUrl = task.sourceUrl;
    result.collectionPath = normalizedCollectionPath(task.collectionPath);
    if (!task.sourceUrl.isLocalFile()) {
        return result;
    }
    const QFileInfo sourceInfo(task.sourceUrl.toLocalFile());
    if (!sourceInfo.isFile()) {
        return result;
    }
    if (!supportedSuffixes.contains(sourceInfo.suffix().toLower())) {
        result.status = ImportResult::Unsupported;
        return result;
    }
    if (pathIsWithin(sourceInfo.absoluteFilePath(), rootPath)) {
        result.status = ImportResult::AlreadyManaged;
        result.destinationUrl = QUrl::fromLocalFile(sourceInfo.absoluteFilePath());
        return result;
    }

    const QString destinationDirectory = result.collectionPath.isEmpty()
                                             ? rootPath
                                             : QDir(rootPath).filePath(
                                                   result.collectionPath);
    if (!QFileInfo(destinationDirectory).isDir()
        || !pathIsWithin(destinationDirectory, rootPath)
               && !samePath(destinationDirectory, rootPath)) {
        result.status = ImportResult::UnavailableCollection;
        return result;
    }
    bool sourceReadable = true;
    const QString duplicate = duplicatePath(sourceInfo.absoluteFilePath(),
                                            rootPath,
                                            supportedSuffixes,
                                            &sourceReadable);
    if (!sourceReadable) {
        result.status = ImportResult::ReadFailed;
        return result;
    }
    if (!duplicate.isEmpty()) {
        result.status = ImportResult::Duplicate;
        result.destinationUrl = QUrl::fromLocalFile(duplicate);
        return result;
    }

    const QString destinationPath = uniqueDestination(sourceInfo.absoluteFilePath(),
                                                      destinationDirectory);
    if (!QFile::copy(sourceInfo.absoluteFilePath(), destinationPath)) {
        result.status = ImportResult::CopyFailed;
        return result;
    }
    result.status = ImportResult::Copied;
    result.destinationUrl = QUrl::fromLocalFile(destinationPath);
    return result;
}

QString LibraryFileService::normalizedCollectionPath(const QString &collectionPath)
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
    return path;
}

bool LibraryFileService::validFileName(const QString &name)
{
    if (name.isEmpty() || name.size() > 120
        || name == QLatin1String(".") || name == QLatin1String("..")
        || name.endsWith(u'.') || name.endsWith(u' ')
        || name.compare(metadataDirectoryName, Qt::CaseInsensitive) == 0) {
        return false;
    }
    static const QRegularExpression reservedCharacters(
        QStringLiteral(R"([<>:"/\\|?*\x00-\x1F])"));
    static const QSet<QString> reservedNames = {
        QStringLiteral("CON"), QStringLiteral("PRN"), QStringLiteral("AUX"),
        QStringLiteral("NUL"), QStringLiteral("COM1"), QStringLiteral("COM2"),
        QStringLiteral("COM3"), QStringLiteral("COM4"), QStringLiteral("COM5"),
        QStringLiteral("COM6"), QStringLiteral("COM7"), QStringLiteral("COM8"),
        QStringLiteral("COM9"), QStringLiteral("LPT1"), QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),
        QStringLiteral("LPT6"), QStringLiteral("LPT7"), QStringLiteral("LPT8"),
        QStringLiteral("LPT9")
    };
    return !name.contains(reservedCharacters)
           && !reservedNames.contains(name.section(u'.', 0, 0).toUpper());
}

QString LibraryFileService::errorForImport(const ImportResult &result)
{
    switch (result.status) {
    case ImportResult::InvalidSource:
        return tr("The selected book file is unavailable.");
    case ImportResult::Unsupported:
        return tr("The selected book format is not supported.");
    case ImportResult::UnavailableCollection:
        return tr("The selected destination folder is unavailable.");
    case ImportResult::ReadFailed:
        return tr("Could not read the selected book for duplicate detection.");
    case ImportResult::CopyFailed:
        return tr("Could not copy the selected book into the library.");
    default:
        return {};
    }
}

QString LibraryFileService::absoluteCollectionPath(const QString &collectionPath) const
{
    const QString normalized = normalizedCollectionPath(collectionPath);
    if (!collectionPath.trimmed().isEmpty()
        && collectionPath.trimmed() != QLatin1String(".")
        && collectionPath.trimmed() != QLatin1String("all")
        && normalized.isEmpty()) {
        return {};
    }
    const QString path = normalized.isEmpty()
                             ? m_rootPath
                             : QDir(m_rootPath).absoluteFilePath(normalized);
    if (!samePath(path, m_rootPath) && !pathIsWithin(path, m_rootPath)) {
        return {};
    }
    return cleanAbsolutePath(path);
}

bool LibraryFileService::ensureIdle()
{
    if (m_busy) {
        setError(tr("Wait for the current library operation to finish."));
        return false;
    }
    return true;
}

bool LibraryFileService::ensureAvailable()
{
    if (!available()) {
        setError(tr("The managed library folder is unavailable."));
        return false;
    }
    return true;
}

bool LibraryFileService::ensureManagedFile(const QUrl &sourceUrl, QString *filePath)
{
    if (!sourceUrl.isLocalFile() || !isManagedBook(sourceUrl)) {
        setError(tr("Only a book inside the managed library can be changed on disk."));
        return false;
    }
    const QString path = cleanAbsolutePath(sourceUrl.toLocalFile());
    if (!QFileInfo(path).isFile()) {
        setError(tr("The selected book file is unavailable."));
        return false;
    }
    if (filePath) {
        *filePath = path;
    }
    return true;
}

bool LibraryFileService::applyMovedBook(const QUrl &oldUrl,
                                        const QUrl &newUrl,
                                        const QString &collectionPath)
{
    QString repositoryError;
    if (!m_repository->relinkBook(oldUrl, newUrl, &repositoryError)) {
        setError(repositoryError.isEmpty()
                     ? tr("Could not preserve the book state after moving the file.")
                     : repositoryError);
        return false;
    }
    m_repository->setBookCollection(newUrl, collectionPath);
    setLastImportedUrl(newUrl);
    return true;
}

void LibraryFileService::beginNextImport()
{
    if (!m_busy) {
        return;
    }
    if (m_cancellationRequested || m_importQueue.isEmpty()) {
        finishBatch();
        return;
    }
    const ImportTask task = m_importQueue.dequeue();
    m_currentFileName = QFileInfo(task.sourceUrl.toLocalFile()).fileName();
    emit operationProgressChanged();
    m_importWatcher.setFuture(QtConcurrent::run(
        [task, rootPath = m_rootPath,
         suffixes = BookMetadataService::supportedSuffixes()]() {
            return performImport(task, rootPath, suffixes);
        }));
}

void LibraryFileService::handleImportFinished()
{
    const ImportResult result = m_importWatcher.result();
    ++m_operationCompleted;
    if (result.status == ImportResult::Copied) {
        const QString collection = collectionForBook(result.destinationUrl);
        m_repository->registerBook(result.destinationUrl, collection, true, false);
        setLastImportedUrl(result.destinationUrl);
        ++m_importedCount;
    } else if (result.status == ImportResult::AlreadyManaged
               || result.status == ImportResult::Duplicate) {
        const QString collection = collectionForBook(result.destinationUrl);
        m_repository->registerBook(result.destinationUrl, collection, true, false);
        setLastImportedUrl(result.destinationUrl);
        ++m_duplicateCount;
        emit duplicateDetected(result.sourceUrl, result.destinationUrl);
    } else {
        ++m_failedCount;
        const QString error = errorForImport(result);
        if (!error.isEmpty()) {
            m_errorMessage = error;
            emit errorMessageChanged();
        }
    }
    emit operationProgressChanged();
    beginNextImport();
}

void LibraryFileService::finishBatch()
{
    const bool canceled = m_cancellationRequested;
    m_busy = false;
    m_cancellationRequested = false;
    m_currentFileName.clear();
    emit operationStateChanged();
    emit operationProgressChanged();
    if (m_importedCount > 0 || m_duplicateCount > 0) {
        emit activityRecorded(QStringLiteral("booksImported"),
                              QStringLiteral("info"),
                              QStringLiteral("%1/%2")
                                  .arg(m_importedCount + m_duplicateCount)
                                  .arg(m_operationTotal));
        emit libraryChanged();
    }
    if (m_failedCount > 0 && !m_errorMessage.isEmpty()) {
        emit operationFailed(m_errorMessage);
    }
    emit batchFinished(m_importedCount,
                       m_duplicateCount,
                       m_failedCount,
                       canceled);
}

void LibraryFileService::resetBatchState(int total)
{
    m_operationTotal = total;
    m_operationCompleted = 0;
    m_importedCount = 0;
    m_duplicateCount = 0;
    m_failedCount = 0;
    m_cancellationRequested = false;
    m_busy = true;
    setLastImportedUrl({});
    emit operationStateChanged();
    emit operationProgressChanged();
}

void LibraryFileService::setError(const QString &errorMessage)
{
    const QString message = errorMessage.trimmed().isEmpty()
                                ? tr("The library operation failed.")
                                : errorMessage;
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
    emit operationFailed(message);
}

void LibraryFileService::setLastImportedUrl(const QUrl &sourceUrl)
{
    if (m_lastImportedUrl == sourceUrl) {
        return;
    }
    m_lastImportedUrl = sourceUrl;
    emit lastImportedUrlChanged();
}
