#include "customcoverstore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

constexpr qint64 maximumSourceSize = 20 * 1024 * 1024;
constexpr qint64 maximumSourcePixels = 40 * 1000 * 1000;
constexpr int maximumCoverWidth = 720;
constexpr int maximumCoverHeight = 1040;

QString defaultLocalDirectory()
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) {
        root = QDir::home().filePath(QStringLiteral(".szhbooks"));
    }
    return QDir(root).filePath(QStringLiteral("custom-covers"));
}

QString cleanAbsolutePath(const QString &path)
{
    return path.isEmpty() ? QString() : QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool isWithinRoot(const QString &path, const QString &rootPath)
{
    if (path.isEmpty() || rootPath.isEmpty()) {
        return false;
    }
    const QString relativePath = QDir(rootPath).relativeFilePath(cleanAbsolutePath(path));
    return !QDir::isAbsolutePath(relativePath)
           && relativePath != QLatin1String("..")
           && !relativePath.startsWith(QStringLiteral("../"))
           && !relativePath.startsWith(QStringLiteral("..\\"));
}

} // namespace

CustomCoverStore::CustomCoverStore(const QString &localDirectory)
    : m_localDirectory(localDirectory.isEmpty()
                           ? defaultLocalDirectory()
                           : cleanAbsolutePath(localDirectory))
{
}

void CustomCoverStore::setManagedRootPath(const QString &rootPath)
{
    m_managedRootPath = cleanAbsolutePath(rootPath);
}

QUrl CustomCoverStore::importCover(const QUrl &bookUrl,
                                  const QUrl &imageUrl,
                                  QString *errorMessage) const
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) {
            *errorMessage = message;
        }
        return QUrl();
    };

    if (!bookUrl.isLocalFile() || !imageUrl.isLocalFile()) {
        return fail(tr("Choose a local image file."));
    }
    const QFileInfo sourceInfo(imageUrl.toLocalFile());
    if (!sourceInfo.exists() || !sourceInfo.isFile()
        || sourceInfo.size() <= 0 || sourceInfo.size() > maximumSourceSize) {
        return fail(tr("The selected cover image is unavailable or too large."));
    }

    QImageReader reader(sourceInfo.absoluteFilePath());
    reader.setAutoTransform(true);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid()
        && qint64(sourceSize.width()) * qint64(sourceSize.height()) > maximumSourcePixels) {
        return fail(tr("The selected cover image is unavailable or too large."));
    }
    if (sourceSize.width() > maximumCoverWidth
        || sourceSize.height() > maximumCoverHeight) {
        reader.setScaledSize(sourceSize.scaled(maximumCoverWidth,
                                               maximumCoverHeight,
                                               Qt::KeepAspectRatio));
    }
    QImage image = reader.read();
    if (image.isNull()) {
        return fail(tr("The selected file is not a readable image."));
    }
    if (image.width() > maximumCoverWidth || image.height() > maximumCoverHeight) {
        image = image.scaled(maximumCoverWidth,
                             maximumCoverHeight,
                             Qt::KeepAspectRatio,
                             Qt::SmoothTransformation);
    }

    const QString directory = storageDirectory(bookUrl);
    if (!QDir().mkpath(directory)) {
        return fail(tr("Could not create the custom cover folder."));
    }
    const QString outputPath = QDir(directory).filePath(coverKey(bookUrl)
                                                        + QStringLiteral(".png"));
    QSaveFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly)
        || !image.save(&output, "PNG")
        || !output.commit()) {
        return fail(tr("Could not save the custom cover."));
    }
    if (errorMessage) {
        errorMessage->clear();
    }
    return QUrl::fromLocalFile(outputPath);
}

QString CustomCoverStore::storageDirectory(const QUrl &bookUrl) const
{
    if (bookUrl.isLocalFile()
        && isWithinRoot(bookUrl.toLocalFile(), m_managedRootPath)) {
        return QDir(m_managedRootPath).filePath(QStringLiteral(".szhbooks/covers"));
    }
    return m_localDirectory;
}

QString CustomCoverStore::coverKey(const QUrl &bookUrl) const
{
    QString identity = bookUrl.toString(QUrl::FullyEncoded);
    if (bookUrl.isLocalFile()
        && isWithinRoot(bookUrl.toLocalFile(), m_managedRootPath)) {
        identity = QDir(m_managedRootPath).relativeFilePath(
            QFileInfo(bookUrl.toLocalFile()).absoluteFilePath());
        identity.replace(u'\\', u'/');
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
}
