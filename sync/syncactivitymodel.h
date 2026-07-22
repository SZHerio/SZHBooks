#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

class SyncActivityModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        EventRole = Qt::UserRole + 1,
        SeverityRole,
        DetailRole,
        TimestampRole
    };
    Q_ENUM(Role)

    explicit SyncActivityModel(const QString &configurationFilePath,
                               QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void append(const QString &event,
                const QString &severity = QStringLiteral("info"),
                const QString &detail = {},
                const QDateTime &timestamp = QDateTime::currentDateTimeUtc());
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    struct Entry final
    {
        QString event;
        QString severity;
        QString detail;
        QDateTime timestamp;
    };

    void load();
    void save() const;

    QString m_filePath;
    QVector<Entry> m_entries;
};
