#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QUrl>

struct LibraryBook final
{
    QUrl sourceUrl;
    QString sourcePath;
    QString title;
    QString author;
    QString series;
    qreal seriesNumber = 0;
    QStringList genres;
    QStringList tags;
    QString language;
    int publicationYear = 0;
    QString formatName;
    QString collectionPath;
    QString metadataFingerprint;
    QUrl coverUrl;
    QUrl customCoverUrl;
    bool metadataEdited = false;
    qreal progress = 0;
    QDateTime lastOpened;
    bool fileAvailable = false;
    bool cloudPlaceholder = false;
};
