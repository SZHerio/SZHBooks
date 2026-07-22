#pragma once

#include <QCoreApplication>
#include <QString>
#include <QUrl>

class CustomCoverStore final
{
    Q_DECLARE_TR_FUNCTIONS(CustomCoverStore)

public:
    explicit CustomCoverStore(const QString &localDirectory = {});

    void setManagedRootPath(const QString &rootPath);
    QUrl importCover(const QUrl &bookUrl,
                     const QUrl &imageUrl,
                     QString *errorMessage = nullptr) const;

private:
    QString storageDirectory(const QUrl &bookUrl) const;
    QString coverKey(const QUrl &bookUrl) const;

    QString m_localDirectory;
    QString m_managedRootPath;
};
