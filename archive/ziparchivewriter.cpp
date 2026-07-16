#include "ziparchivewriter.h"

#include <QSaveFile>

#include <limits>
#include <utility>

#include <miniz.h>

namespace {

QString normalizedArchivePath(QString path)
{
    path.replace(u'\\', u'/');
    if (path.startsWith(u'/') || path.endsWith(u'/')) {
        return {};
    }

    const QStringList parts = path.split(u'/', Qt::KeepEmptyParts);
    for (const QString &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) {
            return {};
        }
    }
    return parts.join(u'/');
}

QString archiveError(mz_zip_archive *archive)
{
    return QString::fromLatin1(mz_zip_get_error_string(mz_zip_get_last_error(archive)));
}

} // namespace

bool ZipArchiveWriter::addFile(const QString &path, const QByteArray &data)
{
    const QString normalizedPath = normalizedArchivePath(path);
    if (normalizedPath.isEmpty()) {
        setError(QStringLiteral("Invalid archive entry path."));
        return false;
    }
    for (const Entry &entry : std::as_const(m_entries)) {
        if (entry.path == normalizedPath) {
            setError(QStringLiteral("Duplicate archive entry path."));
            return false;
        }
    }

    m_entries.append({normalizedPath, data});
    m_error.clear();
    return true;
}

bool ZipArchiveWriter::writeTo(const QString &archivePath)
{
    if (archivePath.isEmpty() || m_entries.isEmpty()) {
        setError(QStringLiteral("Archive path or contents are empty."));
        return false;
    }

    mz_zip_archive archive{};
    if (!mz_zip_writer_init_heap(&archive, 0, 128 * 1024)) {
        setError(archiveError(&archive));
        return false;
    }

    bool success = true;
    for (const Entry &entry : std::as_const(m_entries)) {
        const QByteArray encodedPath = entry.path.toUtf8();
        if (!mz_zip_writer_add_mem(&archive,
                                   encodedPath.constData(),
                                   entry.data.constData(),
                                   static_cast<size_t>(entry.data.size()),
                                   MZ_BEST_COMPRESSION)) {
            setError(archiveError(&archive));
            success = false;
            break;
        }
    }

    void *archiveData = nullptr;
    size_t archiveSize = 0;
    if (success
        && !mz_zip_writer_finalize_heap_archive(&archive,
                                                &archiveData,
                                                &archiveSize)) {
        setError(archiveError(&archive));
        success = false;
    }

    if (success
        && archiveSize > static_cast<size_t>(std::numeric_limits<qint64>::max())) {
        setError(QStringLiteral("Archive is too large to write."));
        success = false;
    }

    if (success) {
        QSaveFile output(archivePath);
        if (!output.open(QIODevice::WriteOnly)
            || output.write(static_cast<const char *>(archiveData),
                            static_cast<qint64>(archiveSize))
                   != static_cast<qint64>(archiveSize)
            || !output.commit()) {
            setError(output.errorString());
            success = false;
        }
    }

    mz_free(archiveData);
    mz_zip_writer_end(&archive);
    if (success) {
        m_error.clear();
    }
    return success;
}

QString ZipArchiveWriter::errorString() const
{
    return m_error;
}

void ZipArchiveWriter::setError(const QString &error)
{
    m_error = error.trimmed().isEmpty()
                  ? QStringLiteral("Could not create ZIP archive.")
                  : error;
}
