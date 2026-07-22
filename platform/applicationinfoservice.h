#pragma once

#include "applicationpaths.h"

#include <QObject>
#include <QUrl>

class ApplicationInfoService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString systemArchitecture READ systemArchitecture CONSTANT)
    Q_PROPERTY(bool portableMode READ portableMode CONSTANT)
    Q_PROPERTY(QString profileDirectoryPath READ profileDirectoryPath CONSTANT)
    Q_PROPERTY(QUrl profileDirectoryUrl READ profileDirectoryUrl CONSTANT)
    Q_PROPERTY(bool thirdPartyNoticesAvailable READ thirdPartyNoticesAvailable CONSTANT)
    Q_PROPERTY(QString copyrightNotice READ copyrightNotice CONSTANT)

public:
    explicit ApplicationInfoService(const ApplicationPaths &paths,
                                    const QString &profileDirectory,
                                    QObject *parent = nullptr);

    QString applicationVersion() const;
    QString qtVersion() const;
    QString systemArchitecture() const;
    bool portableMode() const;
    QString profileDirectoryPath() const;
    QUrl profileDirectoryUrl() const;
    bool thirdPartyNoticesAvailable() const;
    QString copyrightNotice() const;

    Q_INVOKABLE bool openProfileDirectory();
    Q_INVOKABLE bool openThirdPartyNotices();

signals:
    void operationFailed(const QString &errorMessage);

private:
    bool openLocalPath(const QString &path, const QString &failureMessage);

    ApplicationPaths m_paths;
    QString m_profileDirectory;
    QString m_thirdPartyNoticesPath;
};
