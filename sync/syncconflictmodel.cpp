#include "syncconflictmodel.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUrl>

#include <algorithm>
#include <utility>

namespace {

constexpr qint64 maximumConflictFileSize = 16 * 1024 * 1024;
const QString conflictsRelativePath = QStringLiteral(".szhbooks/conflicts");

QString conflictId(const QString &filePath, const QString &key)
{
    QByteArray identity = QFileInfo(filePath).absoluteFilePath().toUtf8();
    identity.append('\0');
    identity.append(key.toUtf8());
    return QString::fromLatin1(
        QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex());
}

bool readObject(const QString &filePath, QJsonObject *object)
{
    const QFileInfo info(filePath);
    if (!info.isFile() || info.size() <= 0 || info.size() > maximumConflictFileSize) {
        return false;
    }
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }
    *object = document.object();
    return object->value(QStringLiteral("product")).toString() == QLatin1String("SZHBooks")
           && object->value(QStringLiteral("schemaVersion")).toInt() == 1;
}

} // namespace

SyncConflictModel::SyncConflictModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SyncConflictModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant SyncConflictModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case ConflictIdRole:
        return entry.id;
    case KeyRole:
        return entry.key;
    case LocalValueRole:
        return displayValue(entry.localValue, entry.localPresent);
    case RemoteValueRole:
        return displayValue(entry.remoteValue, entry.remotePresent);
    case LocalPresentRole:
        return entry.localPresent;
    case RemotePresentRole:
        return entry.remotePresent;
    case CreatedAtRole:
        return entry.createdAt;
    case SourceFileRole:
        return QUrl::fromLocalFile(entry.sourceFile);
    default:
        return {};
    }
}

QHash<int, QByteArray> SyncConflictModel::roleNames() const
{
    return {
        {ConflictIdRole, "conflictId"},
        {KeyRole, "settingKey"},
        {LocalValueRole, "localValue"},
        {RemoteValueRole, "remoteValue"},
        {LocalPresentRole, "localPresent"},
        {RemotePresentRole, "remotePresent"},
        {CreatedAtRole, "createdAt"},
        {SourceFileRole, "sourceFile"}
    };
}

void SyncConflictModel::load(const QString &libraryRootPath)
{
    QVector<Entry> entries;
    const QString rootPath = QDir::cleanPath(QFileInfo(libraryRootPath).absoluteFilePath());
    const QDir directory(QDir(rootPath).filePath(conflictsRelativePath));
    const QFileInfoList files = directory.entryInfoList(
        {QStringLiteral("*.json")}, QDir::Files, QDir::Time | QDir::Reversed);
    for (const QFileInfo &file : files) {
        QJsonObject object;
        if (!readObject(file.absoluteFilePath(), &object)) {
            continue;
        }
        const QJsonArray keys = object.value(QStringLiteral("conflictKeys")).toArray();
        const QJsonObject local = object.value(QStringLiteral("localSettings")).toObject();
        const QJsonObject remote = object.value(QStringLiteral("remoteSettings")).toObject();
        const QDateTime createdAt = QDateTime::fromString(
            object.value(QStringLiteral("createdAt")).toString(), Qt::ISODateWithMs);
        for (const QJsonValue &keyValue : keys) {
            const QString key = keyValue.toString();
            if (key.isEmpty()) {
                continue;
            }
            entries.append({conflictId(file.absoluteFilePath(), key),
                            key,
                            local.value(key).toVariant(),
                            remote.value(key).toVariant(),
                            local.contains(key),
                            remote.contains(key),
                            createdAt,
                            file.absoluteFilePath()});
        }
    }

    const bool countWasChanged = entries.size() != m_entries.size();
    beginResetModel();
    m_entries = std::move(entries);
    m_rootPath = rootPath;
    endResetModel();
    if (countWasChanged) {
        emit countChanged();
    }
}

bool SyncConflictModel::choice(const QString &conflictId,
                               bool useRemote,
                               SyncConflictChoice *choice) const
{
    if (!choice) {
        return false;
    }
    for (const Entry &entry : m_entries) {
        if (entry.id != conflictId) {
            continue;
        }
        choice->id = entry.id;
        choice->key = entry.key;
        choice->value = useRemote ? entry.remoteValue : entry.localValue;
        choice->present = useRemote ? entry.remotePresent : entry.localPresent;
        return true;
    }
    return false;
}

bool SyncConflictModel::markResolved(const QString &conflictId,
                                     const QString &resolution,
                                     QString *errorMessage)
{
    const auto entryIterator = std::find_if(
        m_entries.cbegin(), m_entries.cend(), [&conflictId](const Entry &entry) {
            return entry.id == conflictId;
        });
    if (entryIterator == m_entries.cend()) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate(
                "SyncConflictModel", "This synchronization conflict is no longer available.");
        }
        return false;
    }

    QJsonObject object;
    if (!readObject(entryIterator->sourceFile, &object)) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate(
                "SyncConflictModel", "Could not read the synchronization conflict.");
        }
        return false;
    }

    QJsonArray remainingKeys;
    const QJsonArray keys = object.value(QStringLiteral("conflictKeys")).toArray();
    for (const QJsonValue &key : keys) {
        if (key.toString() != entryIterator->key) {
            remainingKeys.append(key);
        }
    }
    object.insert(QStringLiteral("conflictKeys"), remainingKeys);
    QJsonObject resolutions = object.value(QStringLiteral("resolutions")).toObject();
    resolutions.insert(entryIterator->key,
                       QJsonObject{
                           {QStringLiteral("choice"), resolution},
                           {QStringLiteral("resolvedAt"),
                            QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}
                       });
    object.insert(QStringLiteral("resolutions"), resolutions);

    QSaveFile file(entryIterator->sourceFile);
    const QByteArray data = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(data) != data.size()
        || !file.commit()) {
        if (errorMessage) {
            *errorMessage = QCoreApplication::translate(
                "SyncConflictModel", "Could not update the synchronization conflict.");
        }
        return false;
    }

    load(m_rootPath);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

QString SyncConflictModel::displayValue(const QVariant &value, bool present)
{
    if (!present) {
        return QCoreApplication::translate("SyncConflictModel", "Not set");
    }
    if (value.metaType().id() == QMetaType::QVariantMap
        || value.metaType().id() == QMetaType::QVariantList
        || value.metaType().id() == QMetaType::QStringList) {
        const QJsonValue jsonValue = QJsonValue::fromVariant(value);
        const QByteArray data = jsonValue.isObject()
                                    ? QJsonDocument(jsonValue.toObject()).toJson(
                                          QJsonDocument::Compact)
                                    : QJsonDocument(jsonValue.toArray()).toJson(
                                          QJsonDocument::Compact);
        return QString::fromUtf8(data).left(240);
    }
    const QString text = value.toString();
    return text.isEmpty()
               ? QCoreApplication::translate("SyncConflictModel", "Empty value")
               : text.left(240);
}
