#pragma once

#include <QString>
#include <QStringList>
#include <QUrl>

#include <optional>

struct BookMetadataPatch final
{
    std::optional<QString> title;
    std::optional<QString> author;
    std::optional<QString> series;
    std::optional<qreal> seriesNumber;
    std::optional<QStringList> genres;
    std::optional<QStringList> tags;
    std::optional<QString> language;
    std::optional<int> publicationYear;
    std::optional<QUrl> customCoverUrl;

    bool isEmpty() const
    {
        return !title && !author && !series && !seriesNumber && !genres && !tags
               && !language && !publicationYear && !customCoverUrl;
    }
};
