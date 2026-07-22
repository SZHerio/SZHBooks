#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVariant>
#include <QVector>

struct SyncConflictChoice final
{
    QString id;
    QString key;
    QVariant value;
    bool present = false;
};

class SyncConflictModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Role {
        ConflictIdRole = Qt::UserRole + 1,
        KeyRole,
        LocalValueRole,
        RemoteValueRole,
        LocalPresentRole,
        RemotePresentRole,
        CreatedAtRole,
        SourceFileRole
    };
    Q_ENUM(Role)

    explicit SyncConflictModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void load(const QString &libraryRootPath);
    bool choice(const QString &conflictId,
                bool useRemote,
                SyncConflictChoice *choice) const;
    bool markResolved(const QString &conflictId,
                      const QString &resolution,
                      QString *errorMessage = nullptr);

signals:
    void countChanged();

private:
    struct Entry final
    {
        QString id;
        QString key;
        QVariant localValue;
        QVariant remoteValue;
        bool localPresent = false;
        bool remotePresent = false;
        QDateTime createdAt;
        QString sourceFile;
    };

    static QString displayValue(const QVariant &value, bool present);

    QVector<Entry> m_entries;
    QString m_rootPath;
};
