#include "applicationpaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

const QString portableMarkerName = QStringLiteral("portable.flag");

QString absoluteDirectory(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

} // namespace

ApplicationPaths::ApplicationPaths(const QString &applicationDirectory,
                                   bool portableRequested)
    : m_applicationDirectory(absoluteDirectory(
          applicationDirectory.isEmpty()
              ? QCoreApplication::applicationDirPath()
              : applicationDirectory))
    , m_portableMode(portableRequested
                     || QFileInfo::exists(
                         QDir(m_applicationDirectory).filePath(portableMarkerName)))
{
    if (m_portableMode) {
        m_dataDirectory = QDir(m_applicationDirectory).filePath(QStringLiteral("data"));
    } else {
        m_dataDirectory = standardDirectory(QStandardPaths::AppDataLocation,
                                            QStringLiteral(".szhbooks"));
    }
}

bool ApplicationPaths::portableMode() const
{
    return m_portableMode;
}

QString ApplicationPaths::applicationDirectory() const
{
    return m_applicationDirectory;
}

QString ApplicationPaths::dataDirectory() const
{
    return m_dataDirectory;
}

QString ApplicationPaths::settingsFilePath() const
{
    const QString overridePath = environmentFilePath("SZHBOOKS_SETTINGS_FILE");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }
    if (!m_portableMode) {
        return {};
    }
    return QDir(m_dataDirectory).filePath(QStringLiteral("settings.ini"));
}

QString ApplicationPaths::syncSettingsFilePath() const
{
    const QString overridePath = environmentFilePath("SZHBOOKS_SYNC_SETTINGS_FILE");
    if (!overridePath.isEmpty()) {
        return overridePath;
    }
    if (!m_portableMode) {
        return {};
    }
    return QDir(m_dataDirectory).filePath(QStringLiteral("sync.ini"));
}

QString ApplicationPaths::desktopSettingsFilePath() const
{
    return m_portableMode
               ? QDir(m_dataDirectory).filePath(QStringLiteral("window.ini"))
               : QString();
}

QString ApplicationPaths::diagnosticLogDirectory() const
{
    if (m_portableMode) {
        return QDir(m_dataDirectory).filePath(QStringLiteral("logs"));
    }
    return QDir(standardDirectory(QStandardPaths::AppLocalDataLocation,
                                  QStringLiteral(".szhbooks")))
        .filePath(QStringLiteral("logs"));
}

QString ApplicationPaths::coverCacheDirectory() const
{
    if (m_portableMode) {
        return QDir(m_dataDirectory).filePath(QStringLiteral("cache/covers"));
    }
    return QDir(standardDirectory(QStandardPaths::CacheLocation,
                                  QStringLiteral(".cache/SZHBooks")))
        .filePath(QStringLiteral("covers"));
}

QString ApplicationPaths::customCoverDirectory() const
{
    return QDir(m_dataDirectory).filePath(QStringLiteral("custom-covers"));
}

QString ApplicationPaths::documentationDirectory() const
{
    return QDir(m_applicationDirectory).filePath(QStringLiteral("docs"));
}

QString ApplicationPaths::environmentFilePath(const char *variableName)
{
    const QString value = qEnvironmentVariable(variableName);
    return value.isEmpty() ? QString() : QFileInfo(value).absoluteFilePath();
}

QString ApplicationPaths::standardDirectory(int location,
                                            const QString &fallbackDirectoryName)
{
    QString directory = QStandardPaths::writableLocation(
        static_cast<QStandardPaths::StandardLocation>(location));
    if (directory.isEmpty()) {
        directory = QDir::home().filePath(fallbackDirectoryName);
    }
    return absoluteDirectory(directory);
}
