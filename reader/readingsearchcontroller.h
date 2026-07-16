#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class ReadingTextHighlighter;

struct ReadingTextRange {
    int start = -1;
    int length = 0;
};

class ReadingSearchController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString documentText READ documentText WRITE setDocumentText NOTIFY documentTextChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int resultCount READ resultCount NOTIFY resultsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentStart READ currentStart NOTIFY currentResultChanged)
    Q_PROPERTY(int currentLength READ currentLength NOTIFY currentResultChanged)

public:
    explicit ReadingSearchController(QObject *parent = nullptr);
    ~ReadingSearchController() override;

    QString documentText() const;
    QString query() const;
    int resultCount() const;
    int currentIndex() const;
    int currentStart() const;
    int currentLength() const;

    void setDocumentText(const QString &documentText);
    void setQuery(const QString &query);

    Q_INVOKABLE void attachTextDocument(QObject *quickTextDocument);
    Q_INVOKABLE void setAnnotationRanges(const QVariantList &ranges);
    Q_INVOKABLE void setHighlightColors(const QColor &annotationColor,
                                        const QColor &searchColor,
                                        const QColor &currentSearchColor,
                                        const QColor &currentSearchTextColor);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void clear();

signals:
    void documentTextChanged();
    void queryChanged();
    void resultsChanged();
    void currentIndexChanged();
    void currentResultChanged();

private:
    void rebuildResults();
    void setCurrentIndex(int currentIndex);
    void updateHighlighter();

    QString m_documentText;
    QString m_query;
    QVector<ReadingTextRange> m_results;
    QVector<ReadingTextRange> m_annotations;
    int m_currentIndex = -1;
    ReadingTextHighlighter *m_highlighter = nullptr;
    QColor m_annotationColor = QColor(QStringLiteral("#e2e2e2"));
    QColor m_searchColor = QColor(QStringLiteral("#d0d0d0"));
    QColor m_currentSearchColor = QColor(QStringLiteral("#111111"));
    QColor m_currentSearchTextColor = QColor(QStringLiteral("#ffffff"));
};
