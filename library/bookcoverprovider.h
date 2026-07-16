#pragma once

#include <QString>
#include <QUrl>

class QImage;

class BookCoverProvider final
{
public:
    explicit BookCoverProvider(const QString &cacheDirectory = {});

    QString cacheDirectory() const;
    QUrl cachedCover(const QString &fingerprint) const;
    QUrl storeCover(const QString &fingerprint, const QImage &image) const;

private:
    QString coverPath(const QString &fingerprint) const;

    QString m_cacheDirectory;
};
