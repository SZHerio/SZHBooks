#pragma once

#include <QString>

class ApplicationPaths final
{
public:
    explicit ApplicationPaths(const QString &applicationDirectory = {},
                              bool portableRequested = false);

    bool portableMode() const;
    QString applicationDirectory() const;
    QString dataDirectory() const;
    QString settingsFilePath() const;
    QString syncSettingsFilePath() const;
    QString desktopSettingsFilePath() const;
    QString diagnosticLogDirectory() const;
    QString coverCacheDirectory() const;
    QString customCoverDirectory() const;
    QString documentationDirectory() const;

private:
    static QString environmentFilePath(const char *variableName);
    static QString standardDirectory(int location,
                                     const QString &fallbackDirectoryName);

    QString m_applicationDirectory;
    QString m_dataDirectory;
    bool m_portableMode = false;
};
