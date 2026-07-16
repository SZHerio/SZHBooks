#pragma once

#include <QDateTime>
#include <QString>

enum class ReadingAnnotationType {
    Bookmark,
    Highlight
};

struct ReadingAnnotation {
    QString id;
    ReadingAnnotationType type = ReadingAnnotationType::Bookmark;
    qreal progress = 0;
    int page = -1;
    int start = -1;
    int length = 0;
    QString label;
    QString excerpt;
    QString note;
    QDateTime createdAt;
};
