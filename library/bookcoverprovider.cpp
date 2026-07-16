#include "bookcoverprovider.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

constexpr int maximumCoverWidth = 720;
constexpr int maximumCoverHeight = 1040;

QString defaultCacheDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (root.isEmpty()) {
        root = QDir::home().filePath(QStringLiteral(".cache/SZHBooks"));
    }
    return QDir(root).filePath(QStringLiteral("covers"));
}

} // namespace

BookCoverProvider::BookCoverProvider(const QString &cacheDirectory)
    : m_cacheDirectory(cacheDirectory.isEmpty()
                           ? defaultCacheDirectory()
                           : QFileInfo(cacheDirectory).absoluteFilePath())
{
}

QString BookCoverProvider::cacheDirectory() const
{
    return m_cacheDirectory;
}

QUrl BookCoverProvider::cachedCover(const QString &fingerprint) const
{
    const QString path = coverPath(fingerprint);
    return !path.isEmpty() && QFileInfo::exists(path) ? QUrl::fromLocalFile(path) : QUrl();
}

QUrl BookCoverProvider::storeCover(const QString &fingerprint, const QImage &image) const
{
    if (fingerprint.isEmpty() || image.isNull() || !QDir().mkpath(m_cacheDirectory)) {
        return {};
    }

    QImage output = image;
    if (output.width() > maximumCoverWidth || output.height() > maximumCoverHeight) {
        output = output.scaled(maximumCoverWidth,
                               maximumCoverHeight,
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation);
    }

    const QString path = coverPath(fingerprint);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || !output.save(&file, "PNG")
        || !file.commit()) {
        return {};
    }
    return QUrl::fromLocalFile(path);
}

QString BookCoverProvider::coverPath(const QString &fingerprint) const
{
    if (fingerprint.isEmpty()) {
        return {};
    }
    return QDir(m_cacheDirectory).filePath(fingerprint + QStringLiteral(".png"));
}
