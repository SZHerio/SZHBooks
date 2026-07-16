#include "profilearchiveservice.h"

#include "profilestorage.h"
#include "../archive/ziparchivereader.h"
#include "../archive/ziparchivewriter.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>

namespace {

constexpr int profileSchemaVersion = 1;
constexpr qint64 maximumBackupFileSize = 32 * 1024 * 1024;
constexpr qsizetype maximumJsonEntrySize = 8 * 1024 * 1024;
constexpr qsizetype maximumSettingCount = 100000;
constexpr qsizetype maximumSettingKeyLength = 512;

const QString manifestPath = QStringLiteral("manifest.json");
const QString profilePath = QStringLiteral("profile.json");

QByteArray sha256(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex();
}

QByteArray compactJson(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QJsonObject parsedObject(const QByteArray &data, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (errorMessage) {
            *errorMessage = parseError.error == QJsonParseError::NoError
                                ? QStringLiteral("JSON root is not an object.")
                                : parseError.errorString();
        }
        return {};
    }
    return document.object();
}

bool validSettings(const QJsonObject &settings)
{
    if (settings.size() > maximumSettingCount) {
        return false;
    }
    for (auto entry = settings.constBegin(); entry != settings.constEnd(); ++entry) {
        if (entry.key().isEmpty()
            || entry.key().size() > maximumSettingKeyLength
            || entry.key().contains(QChar::Null)) {
            return false;
        }
    }
    return true;
}

} // namespace

ProfileArchiveService::ProfileArchiveService(ProfileStorage *storage,
                                             QObject *parent)
    : QObject(parent)
    , m_storage(storage)
{
    Q_ASSERT(m_storage);
}

QString ProfileArchiveService::errorMessage() const
{
    return m_errorMessage;
}

bool ProfileArchiveService::exportProfile(const QUrl &destinationUrl)
{
    clearError();
    const QString destinationPath = exportPath(destinationUrl);
    if (destinationPath.isEmpty()) {
        fail(tr("Choose a local destination for the backup."));
        return false;
    }

    const QJsonObject profile{
        {QStringLiteral("schemaVersion"), profileSchemaVersion},
        {QStringLiteral("settings"),
         QJsonObject::fromVariantMap(m_storage->profileValues())}
    };
    const QByteArray profileData = compactJson(profile);
    const QJsonObject manifest{
        {QStringLiteral("product"), QStringLiteral("SZHBooks")},
        {QStringLiteral("schemaVersion"), profileSchemaVersion},
        {QStringLiteral("createdAt"),
         QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {QStringLiteral("applicationVersion"),
         QCoreApplication::applicationVersion()},
        {QStringLiteral("profileEntry"), profilePath},
        {QStringLiteral("profileSha256"), QString::fromLatin1(sha256(profileData))}
    };

    ZipArchiveWriter archive;
    if (!archive.addFile(manifestPath, compactJson(manifest))
        || !archive.addFile(profilePath, profileData)
        || !archive.writeTo(destinationPath)) {
        fail(tr("Could not create the profile backup: %1")
                 .arg(archive.errorString()));
        return false;
    }

    const QUrl exportedUrl = QUrl::fromLocalFile(destinationPath);
    emit profileExported(exportedUrl);
    return true;
}

bool ProfileArchiveService::importProfile(const QUrl &sourceUrl)
{
    clearError();
    if (!sourceUrl.isLocalFile()) {
        fail(tr("Choose a local SZHBooks backup."));
        return false;
    }

    const QFileInfo sourceFile(sourceUrl.toLocalFile());
    if (!sourceFile.exists() || !sourceFile.isFile()) {
        fail(tr("The selected backup does not exist."));
        return false;
    }
    if (sourceFile.size() <= 0 || sourceFile.size() > maximumBackupFileSize) {
        fail(tr("The selected backup is empty or too large."));
        return false;
    }

    const ZipArchiveReader archive(sourceFile.absoluteFilePath());
    if (!archive.isOpen()) {
        fail(tr("Could not open the profile backup: %1")
                 .arg(archive.errorString()));
        return false;
    }

    const QStringList entries = archive.filePaths();
    const QSet<QString> uniqueEntries(entries.cbegin(), entries.cend());
    if (entries.size() != uniqueEntries.size()
        || !uniqueEntries.contains(manifestPath)
        || !uniqueEntries.contains(profilePath)) {
        fail(tr("This is not a valid SZHBooks profile backup."));
        return false;
    }

    const QByteArray manifestData = archive.fileData(manifestPath);
    const QByteArray profileData = archive.fileData(profilePath);
    if (manifestData.isEmpty() || profileData.isEmpty()
        || manifestData.size() > maximumJsonEntrySize
        || profileData.size() > maximumJsonEntrySize) {
        fail(tr("The profile backup is incomplete or too large."));
        return false;
    }

    QString parseError;
    const QJsonObject manifest = parsedObject(manifestData, &parseError);
    if (manifest.isEmpty()
        || manifest.value(QStringLiteral("product")).toString()
               != QLatin1String("SZHBooks")
        || manifest.value(QStringLiteral("schemaVersion")).toInt()
               != profileSchemaVersion
        || manifest.value(QStringLiteral("profileEntry")).toString()
               != profilePath
        || manifest.value(QStringLiteral("profileSha256")).toString().toLatin1()
               != sha256(profileData)) {
        fail(tr("The profile backup manifest is invalid or damaged."));
        return false;
    }

    const QJsonObject profile = parsedObject(profileData, &parseError);
    const QJsonValue settingsValue = profile.value(QStringLiteral("settings"));
    if (profile.isEmpty()
        || profile.value(QStringLiteral("schemaVersion")).toInt()
               != profileSchemaVersion
        || !settingsValue.isObject()
        || !validSettings(settingsValue.toObject())) {
        fail(tr("The profile data is invalid or unsupported."));
        return false;
    }

    QString storageError;
    if (!m_storage->replaceProfileValues(settingsValue.toObject().toVariantMap(),
                                         &storageError)) {
        fail(tr("Could not restore the local profile: %1").arg(storageError));
        return false;
    }

    emit profileImported(QUrl::fromLocalFile(sourceFile.absoluteFilePath()));
    return true;
}

void ProfileArchiveService::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorMessageChanged();
}

QString ProfileArchiveService::exportPath(const QUrl &destinationUrl)
{
    if (!destinationUrl.isLocalFile()) {
        return {};
    }

    QString path = QFileInfo(destinationUrl.toLocalFile()).absoluteFilePath();
    if (!path.endsWith(QStringLiteral(".szhbackup"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".szhbackup");
    }
    return path;
}

void ProfileArchiveService::fail(const QString &errorMessage)
{
    m_errorMessage = errorMessage.trimmed().isEmpty()
                         ? tr("The profile operation failed.")
                         : errorMessage;
    emit errorMessageChanged();
    emit operationFailed(m_errorMessage);
}
