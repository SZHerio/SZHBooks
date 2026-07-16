#include "readingsearchcontroller.h"

#include <QQuickTextDocument>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QVariantMap>

#include <algorithm>

class ReadingTextHighlighter final : public QSyntaxHighlighter
{
public:
    explicit ReadingTextHighlighter(QObject *parent)
        : QSyntaxHighlighter(parent)
    {
    }

    void setRanges(const QVector<ReadingTextRange> &annotations,
                   const QVector<ReadingTextRange> &searchResults,
                   int currentSearchIndex)
    {
        m_annotations = annotations;
        m_searchResults = searchResults;
        m_currentSearchIndex = currentSearchIndex;
        rehighlight();
    }

    void setColors(const QColor &annotationColor,
                   const QColor &searchColor,
                   const QColor &currentSearchColor,
                   const QColor &currentSearchTextColor)
    {
        m_annotationFormat.setBackground(annotationColor);
        m_annotationFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        m_annotationFormat.setUnderlineColor(annotationColor.darker(135));
        m_searchFormat.setBackground(searchColor);
        m_currentSearchFormat.setBackground(currentSearchColor);
        m_currentSearchFormat.setForeground(currentSearchTextColor);
        rehighlight();
    }

protected:
    void highlightBlock(const QString &) override
    {
        const int blockStart = currentBlock().position();
        const int blockEnd = blockStart + currentBlock().length();
        applyRanges(m_annotations, blockStart, blockEnd, m_annotationFormat);
        applyRanges(m_searchResults, blockStart, blockEnd, m_searchFormat);

        if (m_currentSearchIndex >= 0
            && m_currentSearchIndex < m_searchResults.size()) {
            applyRange(m_searchResults.at(m_currentSearchIndex),
                       blockStart,
                       blockEnd,
                       m_currentSearchFormat);
        }
    }

private:
    void applyRanges(const QVector<ReadingTextRange> &ranges,
                     int blockStart,
                     int blockEnd,
                     const QTextCharFormat &format)
    {
        for (const ReadingTextRange &range : ranges) {
            if (range.start >= blockEnd) {
                break;
            }
            applyRange(range, blockStart, blockEnd, format);
        }
    }

    void applyRange(const ReadingTextRange &range,
                    int blockStart,
                    int blockEnd,
                    const QTextCharFormat &format)
    {
        const int rangeEnd = range.start + range.length;
        const int overlapStart = qMax(blockStart, range.start);
        const int overlapEnd = qMin(blockEnd, rangeEnd);
        if (overlapStart < overlapEnd) {
            setFormat(overlapStart - blockStart, overlapEnd - overlapStart, format);
        }
    }

    QVector<ReadingTextRange> m_annotations;
    QVector<ReadingTextRange> m_searchResults;
    int m_currentSearchIndex = -1;
    QTextCharFormat m_annotationFormat;
    QTextCharFormat m_searchFormat;
    QTextCharFormat m_currentSearchFormat;
};

ReadingSearchController::ReadingSearchController(QObject *parent)
    : QObject(parent)
    , m_highlighter(new ReadingTextHighlighter(this))
{
    m_highlighter->setColors(m_annotationColor,
                             m_searchColor,
                             m_currentSearchColor,
                             m_currentSearchTextColor);
}

ReadingSearchController::~ReadingSearchController() = default;

QString ReadingSearchController::documentText() const
{
    return m_documentText;
}

QString ReadingSearchController::query() const
{
    return m_query;
}

int ReadingSearchController::resultCount() const
{
    return m_results.size();
}

int ReadingSearchController::currentIndex() const
{
    return m_currentIndex;
}

int ReadingSearchController::currentStart() const
{
    return m_currentIndex >= 0 && m_currentIndex < m_results.size()
               ? m_results.at(m_currentIndex).start
               : -1;
}

int ReadingSearchController::currentLength() const
{
    return m_currentIndex >= 0 && m_currentIndex < m_results.size()
               ? m_results.at(m_currentIndex).length
               : 0;
}

void ReadingSearchController::setDocumentText(const QString &documentText)
{
    if (m_documentText == documentText) {
        return;
    }

    m_documentText = documentText;
    emit documentTextChanged();
    rebuildResults();
}

void ReadingSearchController::setQuery(const QString &query)
{
    if (m_query == query) {
        return;
    }

    m_query = query;
    emit queryChanged();
    rebuildResults();
}

void ReadingSearchController::attachTextDocument(QObject *quickTextDocument)
{
    QQuickTextDocument *textDocument = qobject_cast<QQuickTextDocument *>(quickTextDocument);
    m_highlighter->setDocument(textDocument ? textDocument->textDocument() : nullptr);
    updateHighlighter();
}

void ReadingSearchController::setAnnotationRanges(const QVariantList &ranges)
{
    QVector<ReadingTextRange> parsedRanges;
    parsedRanges.reserve(ranges.size());
    for (const QVariant &value : ranges) {
        const QVariantMap range = value.toMap();
        const int start = range.value(QStringLiteral("start"), -1).toInt();
        const int length = range.value(QStringLiteral("length"), 0).toInt();
        if (start >= 0 && length > 0) {
            parsedRanges.append({start, length});
        }
    }
    std::sort(parsedRanges.begin(),
              parsedRanges.end(),
              [](const ReadingTextRange &left, const ReadingTextRange &right) {
                  return left.start < right.start;
              });
    m_annotations = parsedRanges;
    updateHighlighter();
}

void ReadingSearchController::setHighlightColors(const QColor &annotationColor,
                                                  const QColor &searchColor,
                                                  const QColor &currentSearchColor,
                                                  const QColor &currentSearchTextColor)
{
    m_annotationColor = annotationColor;
    m_searchColor = searchColor;
    m_currentSearchColor = currentSearchColor;
    m_currentSearchTextColor = currentSearchTextColor;
    m_highlighter->setColors(m_annotationColor,
                             m_searchColor,
                             m_currentSearchColor,
                             m_currentSearchTextColor);
}

void ReadingSearchController::next()
{
    if (m_results.isEmpty()) {
        return;
    }
    setCurrentIndex((m_currentIndex + 1) % m_results.size());
}

void ReadingSearchController::previous()
{
    if (m_results.isEmpty()) {
        return;
    }
    setCurrentIndex((m_currentIndex - 1 + m_results.size()) % m_results.size());
}

void ReadingSearchController::clear()
{
    setQuery(QString());
}

void ReadingSearchController::rebuildResults()
{
    const int previousIndex = m_currentIndex;
    m_results.clear();
    const QString needle = m_query.trimmed();
    if (!needle.isEmpty() && !m_documentText.isEmpty()) {
        int offset = 0;
        while (offset < m_documentText.size()) {
            const int match = m_documentText.indexOf(needle, offset, Qt::CaseInsensitive);
            if (match < 0) {
                break;
            }
            m_results.append({match, static_cast<int>(needle.size())});
            offset = match + qMax(1, needle.size());
        }
    }

    m_currentIndex = m_results.isEmpty() ? -1 : 0;
    emit resultsChanged();
    if (m_currentIndex != previousIndex) {
        emit currentIndexChanged();
    }
    emit currentResultChanged();
    updateHighlighter();
}

void ReadingSearchController::setCurrentIndex(int currentIndex)
{
    if (m_results.isEmpty()) {
        currentIndex = -1;
    } else {
        currentIndex = qBound(0, currentIndex, m_results.size() - 1);
    }
    if (m_currentIndex == currentIndex) {
        return;
    }

    m_currentIndex = currentIndex;
    emit currentIndexChanged();
    emit currentResultChanged();
    updateHighlighter();
}

void ReadingSearchController::updateHighlighter()
{
    m_highlighter->setRanges(m_annotations, m_results, m_currentIndex);
}
