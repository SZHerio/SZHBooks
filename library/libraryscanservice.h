#pragma once

#include "bookmetadata.h"
#include "librarybook.h"

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QThreadPool>
#include <QVector>

class BookMetadataService;

struct LibraryScanResult final
{
    QUrl sourceUrl;
    bool fileAvailable = false;
    bool cloudPlaceholder = false;
    bool metadataInspected = false;
    BookMetadata metadata;
};

Q_DECLARE_METATYPE(LibraryScanResult)

class LibraryScanService final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool cancellationRequested READ cancellationRequested NOTIFY scanningChanged)
    Q_PROPERTY(int completed READ completed NOTIFY progressChanged)
    Q_PROPERTY(int total READ total NOTIFY progressChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)

public:
    explicit LibraryScanService(const BookMetadataService *metadataService,
                                QObject *parent = nullptr);
    ~LibraryScanService() override;

    bool scanning() const;
    bool cancellationRequested() const;
    int completed() const;
    int total() const;
    qreal progress() const;

    void start(const QVector<LibraryBook> &books);
    Q_INVOKABLE void cancel();

signals:
    void scanningChanged();
    void progressChanged();
    void resultsReady(const QVector<LibraryScanResult> &results);
    void finished(bool canceled);

private:
    static LibraryScanResult scanBook(const LibraryBook &book,
                                      const BookMetadataService *metadataService);
    void queueResults(int begin, int end);
    void flushResults();
    void completeRun();

    const BookMetadataService *m_metadataService = nullptr;
    QFutureWatcher<LibraryScanResult> m_watcher;
    QThreadPool m_threadPool;
    QTimer m_resultTimer;
    QVector<LibraryScanResult> m_pendingResults;
    QVector<LibraryBook> m_restartBooks;
    qsizetype m_pendingResultOffset = 0;
    int m_completed = 0;
    int m_total = 0;
    bool m_scanning = false;
    bool m_cancellationRequested = false;
    bool m_workerFinished = false;
    bool m_restartPending = false;
};
