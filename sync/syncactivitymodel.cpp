#include "syncactivitymodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {

constexpr int maximumEntries = 100;

QString activityFilePath(const QString &configurationFilePath)
{
    const QFileInfo configuration(configurationFilePath);
    return QDir(configuration.absolutePath()).filePath(QStringLiteral("sync-activity.json"));
}

} // namespace

SyncActivityModel::SyncActivityModel(const QString &configurationFilePath, QObject *parent)
    : QAbstractListModel(parent)
    , m_filePath(activityFilePath(configurationFilePath))
{
    load();
}

int SyncActivityModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant SyncActivityModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case EventRole:
        return entry.event;
    case SeverityRole:
        return entry.severity;
    case DetailRole:
        return entry.detail;
    case TimestampRole:
        return entry.timestamp;
    default:
        return {};
    }
}

QHash<int, QByteArray> SyncActivityModel::roleNames() const
{
    return {
        {EventRole, "event"},
        {SeverityRole, "severity"},
        {DetailRole, "detail"},
        {TimestampRole, "timestamp"}
    };
}

void SyncActivityModel::append(const QString &event,
                               const QString &severity,
                               const QString &detail,
                               const QDateTime &timestamp)
{
    if (event.trimmed().isEmpty()) {
        return;
    }

    beginInsertRows({}, 0, 0);
    m_entries.prepend({event.trimmed(), severity.trimmed(), detail.trimmed(), timestamp});
    endInsertRows();
    if (m_entries.size() > maximumEntries) {
        beginRemoveRows({}, maximumEntries, m_entries.size() - 1);
        m_entries.resize(maximumEntries);
        endRemoveRows();
    }
    save();
    emit countChanged();
}

void SyncActivityModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }
    beginResetModel();
    m_entries.clear();
    endResetModel();
    QFile::remove(m_filePath);
    emit countChanged();
}

void SyncActivityModel::load()
{
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray()) {
        return;
    }
    const QJsonArray entries = document.array();
    m_entries.reserve(qMin(entries.size(), maximumEntries));
    for (const QJsonValue &value : entries) {
        if (!value.isObject() || m_entries.size() >= maximumEntries) {
            continue;
        }
        const QJsonObject object = value.toObject();
        const QString event = object.value(QStringLiteral("event")).toString().trimmed();
        const QDateTime timestamp = QDateTime::fromString(
            object.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs);
        if (event.isEmpty() || !timestamp.isValid()) {
            continue;
        }
        m_entries.append({event,
                          object.value(QStringLiteral("severity")).toString(),
                          object.value(QStringLiteral("detail")).toString(),
                          timestamp});
    }
}

void SyncActivityModel::save() const
{
    QJsonArray entries;
    for (const Entry &entry : m_entries) {
        entries.append(QJsonObject{
            {QStringLiteral("event"), entry.event},
            {QStringLiteral("severity"), entry.severity},
            {QStringLiteral("detail"), entry.detail},
            {QStringLiteral("timestamp"),
             entry.timestamp.toUTC().toString(Qt::ISODateWithMs)}
        });
    }

    QDir().mkpath(QFileInfo(m_filePath).absolutePath());
    QSaveFile file(m_filePath);
    const QByteArray data = QJsonDocument(entries).toJson(QJsonDocument::Compact);
    if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size()) {
        file.commit();
    }
}
