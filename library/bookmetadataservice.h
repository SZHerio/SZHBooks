#pragma once

#include "bookmetadata.h"

#include <QStringList>

class BookCoverProvider;
class QUrl;

class BookMetadataService final
{
public:
    explicit BookMetadataService(BookCoverProvider *coverProvider);

    BookMetadata inspect(const QUrl &sourceUrl) const;
    QString fingerprint(const QUrl &sourceUrl) const;
    bool supports(const QUrl &sourceUrl) const;

    static QStringList supportedSuffixes();

private:
    BookCoverProvider *m_coverProvider = nullptr;
};
