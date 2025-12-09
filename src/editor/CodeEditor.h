#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QList>
#include <QSet>
#include <QTextEdit>

#include "XXMLSyntaxHighlighter.h"

namespace XXMLStudio {

class LineNumberArea;

/**
 * Diagnostic information for displaying error/warning underlines.
 */
struct Diagnostic {
    enum class Severity {
        Error,
        Warning,
        Info,
        Hint
    };

    int startLine;      // 1-based
    int startColumn;    // 1-based
    int endLine;
    int endColumn;
    Severity severity;
    QString message;
};

/**
 * Code editor widget with line numbers, current line highlight,
 * syntax highlighting, diagnostics, and bookmark support.
 */
class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget* parent = nullptr);
    ~CodeEditor();

    // File path management
    QString filePath() const { return m_filePath; }
    void setFilePath(const QString& path) { m_filePath = path; }

    // Line number area
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;

    // Editor settings
    void setTabWidth(int spaces);
    void setShowLineNumbers(bool show);
    void setHighlightCurrentLine(bool highlight);
    void setUseSpacesForTabs(bool useSpaces);
    bool useSpacesForTabs() const { return m_useSpacesForTabs; }
    void setAutoClosePairs(bool enable);
    bool autoClosePairs() const { return m_autoClosePairs; }
    void setSyntaxTheme(SyntaxTheme theme);
    SyntaxTheme syntaxTheme() const;

    // Navigation
    void goToLine(int line);
    void goToPosition(int line, int column);

    // Diagnostics (error underlines)
    void setDiagnostics(const QList<Diagnostic>& diagnostics);
    void clearDiagnostics();
    QList<Diagnostic> diagnostics() const { return m_diagnostics; }
    QString diagnosticAt(int line, int column) const;

    // Bookmarks
    void setBookmarkedLines(const QList<int>& lines);
    QList<int> bookmarkedLines() const { return m_bookmarkedLines.values(); }
    bool hasBookmark(int line) const;

    // Find/Replace
    bool find(const QString& text, QTextDocument::FindFlags flags = QTextDocument::FindFlags());
    bool findNext(const QString& text, bool caseSensitive = false, bool wholeWord = false, bool useRegex = false);
    bool findPrevious(const QString& text, bool caseSensitive = false, bool wholeWord = false, bool useRegex = false);
    int replaceAll(const QString& searchText, const QString& replaceText, bool caseSensitive = false, bool wholeWord = false, bool useRegex = false);
    bool replaceCurrent(const QString& replaceText);

    // Selection info
    int currentLine() const;
    int currentColumn() const;

signals:
    void modificationChanged(bool modified);
    void cursorPositionChanged(int line, int column);
    void diagnosticHovered(const QString& message);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect& rect, int dy);
    void onModificationChanged(bool changed);
    void onCursorPositionChanged();

private:
    void setupEditor();
    void setupConnections();
    void paintDiagnostics(QPainter& painter);
    void paintBookmarks(QPainter& painter);
    QTextDocument::FindFlags buildFindFlags(bool caseSensitive, bool wholeWord, bool backward) const;
    void highlightMatchingBracket();
    int findMatchingBracket(int pos, QChar bracket, bool forward) const;

    QString m_filePath;
    LineNumberArea* m_lineNumberArea = nullptr;
    XXMLSyntaxHighlighter* m_highlighter = nullptr;
    bool m_showLineNumbers = true;
    bool m_highlightCurrentLine = true;
    bool m_useSpacesForTabs = true;
    bool m_autoClosePairs = true;
    int m_tabWidth = 4;

    // Diagnostics
    QList<Diagnostic> m_diagnostics;

    // Bookmarks (1-based line numbers)
    QSet<int> m_bookmarkedLines;

    // Find state
    QString m_lastSearchText;
};

/**
 * Line number area widget displayed in the editor's left margin.
 */
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor* editor)
        : QWidget(editor), m_codeEditor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        m_codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor* m_codeEditor;
};

} // namespace XXMLStudio

#endif // CODEEDITOR_H
