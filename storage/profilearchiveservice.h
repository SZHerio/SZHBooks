#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class ProfileStorage;

class ProfileArchiveService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ProfileArchiveService(ProfileStorage *storage,
                                   QObject *parent = nullptr);

    QString errorMessage() const;

    Q_INVOKABLE bool exportProfile(const QUrl &destinationUrl);
    Q_INVOKABLE bool importProfile(const QUrl &sourceUrl);
    Q_INVOKABLE void clearError();

signals:
    void profileExported(const QUrl &destinationUrl);
    void profileImported(const QUrl &sourceUrl);
    void operationFailed(const QString &errorMessage);
    void errorMessageChanged();

private:
    static QString exportPath(const QUrl &destinationUrl);
    void fail(const QString &errorMessage);

    ProfileStorage *m_storage = nullptr;
    QString m_errorMessage;
};
