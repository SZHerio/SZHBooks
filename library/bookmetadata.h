#pragma once

#include <QString>
#include <QUrl>

struct BookMetadata final
{
    QString title;
    QString author;
    QString formatName;
    QString fingerprint;
    QUrl coverUrl;
    bool hasDocumentTitle = false;
    bool hasDocumentAuthor = false;
};
