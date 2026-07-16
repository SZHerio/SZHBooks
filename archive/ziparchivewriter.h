#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

class ZipArchiveWriter final
{
public:
    bool addFile(const QString &path, const QByteArray &data);
    bool writeTo(const QString &archivePath);
    QString errorString() const;

private:
    struct Entry final
    {
        QString path;
        QByteArray data;
    };

    void setError(const QString &error);

    QVector<Entry> m_entries;
    QString m_error;
};
