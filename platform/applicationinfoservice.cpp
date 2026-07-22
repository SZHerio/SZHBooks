#include "applicationinfoservice.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSysInfo>

ApplicationInfoService::ApplicationInfoService(const ApplicationPaths &paths,
                                               const QString &profileDirectory,
                                               QObject *parent)
    : QObject(parent)
    , m_paths(paths)
    , m_profileDirectory(QFileInfo(profileDirectory).absoluteFilePath())
    , m_thirdPartyNoticesPath(
          QDir(paths.documentationDirectory()).filePath(
              QStringLiteral("THIRD_PARTY_NOTICES.md")))
{
}

QString ApplicationInfoService::applicationVersion() const
{
    return QCoreApplication::applicationVersion();
}

QString ApplicationInfoService::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

QString ApplicationInfoService::systemArchitecture() const
{
    return QSysInfo::currentCpuArchitecture();
}

bool ApplicationInfoService::portableMode() const
{
    return m_paths.portableMode();
}

QString ApplicationInfoService::profileDirectoryPath() const
{
    return QDir::toNativeSeparators(m_profileDirectory);
}

QUrl ApplicationInfoService::profileDirectoryUrl() const
{
    return QUrl::fromLocalFile(m_profileDirectory);
}

bool ApplicationInfoService::thirdPartyNoticesAvailable() const
{
    return QFileInfo::exists(m_thirdPartyNoticesPath);
}

QString ApplicationInfoService::copyrightNotice() const
{
    return QStringLiteral("Copyright 2026 SZHerio. All rights reserved.");
}

bool ApplicationInfoService::openProfileDirectory()
{
    if (!QDir().mkpath(m_profileDirectory)) {
        emit operationFailed(tr("The profile folder could not be created."));
        return false;
    }
    return openLocalPath(m_profileDirectory,
                         tr("The profile folder could not be opened."));
}

bool ApplicationInfoService::openThirdPartyNotices()
{
    if (!thirdPartyNoticesAvailable()) {
        emit operationFailed(tr("Third-party notices are unavailable."));
        return false;
    }
    return openLocalPath(m_thirdPartyNoticesPath,
                         tr("Third-party notices could not be opened."));
}

bool ApplicationInfoService::openLocalPath(const QString &path,
                                           const QString &failureMessage)
{
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        return true;
    }
    emit operationFailed(failureMessage);
    return false;
}
