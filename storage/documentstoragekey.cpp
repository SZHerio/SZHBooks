#include "documentstoragekey.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>

namespace DocumentStorageKey {

QString id(const QUrl &documentUrl)
{
    QString identity;
    if (documentUrl.isLocalFile()) {
        const QFileInfo fileInfo(documentUrl.toLocalFile());
        identity = fileInfo.canonicalFilePath();
        if (identity.isEmpty()) {
            identity = fileInfo.absoluteFilePath();
        }
        identity = QDir::cleanPath(identity);
#ifdef Q_OS_WIN
        identity = identity.toLower();
#endif
    } else {
        identity = documentUrl.adjusted(QUrl::NormalizePathSegments | QUrl::RemoveFragment)
                       .toString(QUrl::FullyEncoded);
    }

    return QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString key(const QUrl &documentUrl, const QString &name)
{
    return QStringLiteral("documents/%1/%2").arg(id(documentUrl), name);
}

} // namespace DocumentStorageKey
