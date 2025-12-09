#include "CodeEditor.h"
#include "XXMLSyntaxHighlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QToolTip>
#include <QRegularExpression>

namespace XXMLStudio {

CodeEditor::CodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    setupEditor();
    setupConnections();
}

CodeEditor::~CodeEditor()
{
}

void CodeEditor::setupEditor()
{
    // Create line number area
    m_lineNumberArea = new LineNumberArea(this);

    // Create syntax highlighter
    m_highlighter = new XXMLSyntaxHighlighter(document());

    // Set editor properties
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setMouseTracking(true);  // For diagnostic tooltips

    // Set tab stop width (will be updated with proper font metrics)
    setTabWidth(m_tabWidth);

    // Initial line number area update
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeEditor::setupConnections()
{
    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::onCursorPositionChanged);
    connect(document(), &QTextDocument::modificationChanged,
            this, &CodeEditor::onModificationChanged);
}

int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers) {
        return 0;
    }

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    // Extra space for bookmark markers
    int bookmarkSpace = 16;
    int space = bookmarkSpace + 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly() && m_highlightCurrentLine) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(40, 40, 40);  // Dark theme current line color

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // Bracket matching
    static const QString openBrackets = "([{";
    static const QString closeBrackets = ")]}";

    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    QChar currentChar = document()->characterAt(pos);
    QChar prevChar = pos > 0 ? document()->characterAt(pos - 1) : QChar();

    int bracketPos = -1;
    QChar bracket;
    bool searchForward = true;

    // Check current position or previous position for brackets
    if (openBrackets.contains(currentChar)) {
        bracketPos = pos;
        bracket = currentChar;
        searchForward = true;
    } else if (closeBrackets.contains(currentChar)) {
        bracketPos = pos;
        bracket = currentChar;
        searchForward = false;
    } else if (closeBrackets.contains(prevChar)) {
        bracketPos = pos - 1;
        bracket = prevChar;
        searchForward = false;
    } else if (openBrackets.contains(prevChar)) {
        bracketPos = pos - 1;
        bracket = prevChar;
        searchForward = true;
    }

    if (bracketPos >= 0) {
        int matchPos = findMatchingBracket(bracketPos, bracket, searchForward);
        if (matchPos >= 0) {
            // Highlight both brackets with a subtle background
            QColor bracketMatchColor(80, 80, 80);

            QTextEdit::ExtraSelection sel1;
            sel1.format.setBackground(bracketMatchColor);
            sel1.cursor = QTextCursor(document());
            sel1.cursor.setPosition(bracketPos);
            sel1.cursor.setPosition(bracketPos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel1);

            QTextEdit::ExtraSelection sel2;
            sel2.format.setBackground(bracketMatchColor);
            sel2.cursor = QTextCursor(document());
            sel2.cursor.setPosition(matchPos);
            sel2.cursor.setPosition(matchPos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(sel2);
        }
    }

    // Add diagnostic underlines
    for (const Diagnostic& diag : m_diagnostics) {
        QTextEdit::ExtraSelection selection;

        QColor underlineColor;
        switch (diag.severity) {
            case Diagnostic::Severity::Error:
                underlineColor = QColor(255, 0, 0);
                break;
            case Diagnostic::Severity::Warning:
                underlineColor = QColor(255, 200, 0);
                break;
            case Diagnostic::Severity::Info:
                underlineColor = QColor(0, 150, 255);
                break;
            case Diagnostic::Severity::Hint:
                underlineColor = QColor(100, 100, 100);
                break;
        }

        selection.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        selection.format.setUnderlineColor(underlineColor);

        // Position cursor at diagnostic location
        QTextBlock startBlock = document()->findBlockByLineNumber(diag.startLine - 1);
        QTextBlock endBlock = document()->findBlockByLineNumber(diag.endLine - 1);

        if (startBlock.isValid() && endBlock.isValid()) {
            int startPos = startBlock.position() + diag.startColumn - 1;
            int endPos = endBlock.position() + diag.endColumn - 1;

            // Clamp to block bounds
            startPos = qMax(startPos, startBlock.position());
            endPos = qMin(endPos, endBlock.position() + endBlock.length() - 1);

            if (startPos < endPos) {
                selection.cursor = QTextCursor(document());
                selection.cursor.setPosition(startPos);
                selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
                extraSelections.append(selection);
            }
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    if (!m_showLineNumbers) {
        return;
    }

    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(30, 30, 30));  // Dark theme gutter

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    int bookmarkAreaWidth = 16;

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            int lineNumber = blockNumber + 1;
            QString number = QString::number(lineNumber);

            // Draw bookmark marker
            if (m_bookmarkedLines.contains(lineNumber)) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setBrush(QColor(0, 122, 204));  // Blue bookmark color
                painter.setPen(Qt::NoPen);

                int markerSize = 8;
                int markerX = 4;
                int markerY = top + (fontMetrics().height() - markerSize) / 2;
                painter.drawEllipse(markerX, markerY, markerSize, markerSize);
                painter.restore();
            }

            // Check if line has diagnostic
            bool hasError = false;
            bool hasWarning = false;
            for (const Diagnostic& diag : m_diagnostics) {
                if (diag.startLine == lineNumber) {
                    if (diag.severity == Diagnostic::Severity::Error) {
                        hasError = true;
                    } else if (diag.severity == Diagnostic::Severity::Warning) {
                        hasWarning = true;
                    }
                }
            }

            // Highlight current line number
            if (blockNumber == textCursor().blockNumber()) {
                painter.setPen(QColor(200, 200, 200));  // Bright for current line
            } else if (hasError) {
                painter.setPen(QColor(255, 100, 100));  // Red for errors
            } else if (hasWarning) {
                painter.setPen(QColor(255, 200, 100));  // Yellow for warnings
            } else {
                painter.setPen(QColor(100, 100, 100));  // Dim for other lines
            }

            painter.drawText(bookmarkAreaWidth, top, m_lineNumberArea->width() - bookmarkAreaWidth - 8,
                           fontMetrics().height(),
                           Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }

    // Draw separator line between line numbers and code
    painter.setPen(QColor(62, 62, 66));  // #3e3e42 - subtle gray separator
    int lineX = m_lineNumberArea->width() - 1;
    painter.drawLine(lineX, event->rect().top(), lineX, event->rect().bottom());
}

void CodeEditor::paintEvent(QPaintEvent* event)
{
    QPlainTextEdit::paintEvent(event);
}

void CodeEditor::mouseMoveEvent(QMouseEvent* event)
{
    QPlainTextEdit::mouseMoveEvent(event);

    // Show diagnostic tooltip on hover
    QTextCursor cursor = cursorForPosition(event->pos());
    int line = cursor.blockNumber() + 1;
    int column = cursor.positionInBlock() + 1;

    QString message = diagnosticAt(line, column);
    if (!message.isEmpty()) {
        QToolTip::showText(event->globalPosition().toPoint(), message, this);
    } else {
        QToolTip::hideText();
    }
}

bool CodeEditor::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        QTextCursor cursor = cursorForPosition(helpEvent->pos());
        int line = cursor.blockNumber() + 1;
        int column = cursor.positionInBlock() + 1;

        QString message = diagnosticAt(line, column);
        if (!message.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), message);
        } else {
            QToolTip::hideText();
        }
        return true;
    }
    return QPlainTextEdit::event(event);
}

void CodeEditor::keyPressEvent(QKeyEvent* event)
{
    QString text = event->text();

    // Auto-closing pairs
    if (m_autoClosePairs && text.length() == 1) {
        QChar ch = text[0];
        QTextCursor cursor = textCursor();

        // Define pairs
        static const QMap<QChar, QChar> openPairs = {
            {'(', ')'},
            {'{', '}'},
            {'[', ']'}
        };

        // Opening brackets - insert pair
        if (openPairs.contains(ch)) {
            cursor.insertText(QString(ch) + openPairs[ch]);
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return;
        }

        // Quotes - toggle or insert pair
        if (ch == '"' || ch == '\'') {
            // Check if next char is the same quote - just move past it
            if (!cursor.atEnd()) {
                QChar nextChar = document()->characterAt(cursor.position());
                if (nextChar == ch) {
                    cursor.movePosition(QTextCursor::Right);
                    setTextCursor(cursor);
                    return;
                }
            }
            // Insert pair
            cursor.insertText(QString(ch) + QString(ch));
            cursor.movePosition(QTextCursor::Left);
            setTextCursor(cursor);
            return;
        }

        // Closing brackets - skip over if next char matches
        static const QString closingChars = ")]}";
        if (closingChars.contains(ch)) {
            if (!cursor.atEnd()) {
                QChar nextChar = document()->characterAt(cursor.position());
                if (nextChar == ch) {
                    cursor.movePosition(QTextCursor::Right);
                    setTextCursor(cursor);
                    return;
                }
            }
        }
    }

    // Handle Backspace - delete matching pairs
    if (event->key() == Qt::Key_Backspace && m_autoClosePairs) {
        QTextCursor cursor = textCursor();
        if (!cursor.atStart() && !cursor.atEnd()) {
            int pos = cursor.position();
            QChar prevChar = document()->characterAt(pos - 1);
            QChar nextChar = document()->characterAt(pos);

            // Check if we're between matching pairs
            static const QMap<QChar, QChar> matchingPairs = {
                {'(', ')'},
                {'{', '}'},
                {'[', ']'},
                {'"', '"'},
                {'\'', '\''}
            };

            if (matchingPairs.contains(prevChar) && matchingPairs[prevChar] == nextChar) {
                // Delete both characters
                cursor.movePosition(QTextCursor::Left);
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
                cursor.removeSelectedText();
                return;
            }
        }
    }

    // Handle Tab key for indentation
    if (event->key() == Qt::Key_Tab) {
        QTextCursor cursor = textCursor();
        if (m_useSpacesForTabs) {
            cursor.insertText(QString(m_tabWidth, ' '));
        } else {
            cursor.insertText("\t");
        }
        return;
    }

    // Handle Shift+Tab for unindentation
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, m_tabWidth);
        QString selectedText = cursor.selectedText();

        // Remove leading spaces/tabs (up to tab width)
        int charsToRemove = 0;
        for (int i = 0; i < selectedText.length() && i < m_tabWidth; ++i) {
            if (selectedText[i] == ' ') {
                ++charsToRemove;
            } else if (selectedText[i] == '\t') {
                ++charsToRemove;
                break;  // One tab counts as full tab width
            } else {
                break;
            }
        }

        if (charsToRemove > 0) {
            cursor.setPosition(cursor.position() - selectedText.length());
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, charsToRemove);
            cursor.removeSelectedText();
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::onModificationChanged(bool changed)
{
    emit modificationChanged(changed);
}

void CodeEditor::onCursorPositionChanged()
{
    emit cursorPositionChanged(currentLine(), currentColumn());
}

void CodeEditor::setTabWidth(int spaces)
{
    m_tabWidth = spaces;
    QFontMetrics metrics(font());
    setTabStopDistance(spaces * metrics.horizontalAdvance(' '));
}

void CodeEditor::setShowLineNumbers(bool show)
{
    m_showLineNumbers = show;
    m_lineNumberArea->setVisible(show);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::setHighlightCurrentLine(bool highlight)
{
    m_highlightCurrentLine = highlight;
    highlightCurrentLine();
}

void CodeEditor::setUseSpacesForTabs(bool useSpaces)
{
    m_useSpacesForTabs = useSpaces;
}

void CodeEditor::setAutoClosePairs(bool enable)
{
    m_autoClosePairs = enable;
}

void CodeEditor::setSyntaxTheme(SyntaxTheme theme)
{
    if (m_highlighter) {
        m_highlighter->setTheme(theme);
    }
}

SyntaxTheme CodeEditor::syntaxTheme() const
{
    if (m_highlighter) {
        return m_highlighter->theme();
    }
    return SyntaxTheme::VSCodeDark;
}

void CodeEditor::goToLine(int line)
{
    if (line < 1) line = 1;
    if (line > blockCount()) line = blockCount();

    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    QTextCursor cursor(block);
    setTextCursor(cursor);
    centerCursor();
}

void CodeEditor::goToPosition(int line, int column)
{
    if (line < 1) line = 1;
    if (line > blockCount()) line = blockCount();

    QTextBlock block = document()->findBlockByLineNumber(line - 1);
    QTextCursor cursor(block);

    // Move to column
    int maxColumn = block.length();
    if (column > maxColumn) column = maxColumn;
    if (column > 1) {
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column - 1);
    }

    setTextCursor(cursor);
    centerCursor();
}

// Diagnostics
void CodeEditor::setDiagnostics(const QList<Diagnostic>& diagnostics)
{
    m_diagnostics = diagnostics;
    highlightCurrentLine();  // Refresh extra selections
    m_lineNumberArea->update();  // Refresh line number colors
}

void CodeEditor::clearDiagnostics()
{
    m_diagnostics.clear();
    highlightCurrentLine();
    m_lineNumberArea->update();
}

QString CodeEditor::diagnosticAt(int line, int column) const
{
    for (const Diagnostic& diag : m_diagnostics) {
        if (line >= diag.startLine && line <= diag.endLine) {
            if (line == diag.startLine && column < diag.startColumn) continue;
            if (line == diag.endLine && column > diag.endColumn) continue;
            return diag.message;
        }
    }
    return QString();
}

// Bookmarks
void CodeEditor::setBookmarkedLines(const QList<int>& lines)
{
    m_bookmarkedLines.clear();
    for (int line : lines) {
        m_bookmarkedLines.insert(line);
    }
    m_lineNumberArea->update();
}

bool CodeEditor::hasBookmark(int line) const
{
    return m_bookmarkedLines.contains(line);
}

// Find/Replace
QTextDocument::FindFlags CodeEditor::buildFindFlags(bool caseSensitive, bool wholeWord, bool backward) const
{
    QTextDocument::FindFlags flags;
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (wholeWord) {
        flags |= QTextDocument::FindWholeWords;
    }
    if (backward) {
        flags |= QTextDocument::FindBackward;
    }
    return flags;
}

bool CodeEditor::find(const QString& text, QTextDocument::FindFlags flags)
{
    return QPlainTextEdit::find(text, flags);
}

bool CodeEditor::findNext(const QString& text, bool caseSensitive, bool wholeWord, bool useRegex)
{
    m_lastSearchText = text;

    if (useRegex) {
        QRegularExpression regex(text);
        if (!caseSensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        return QPlainTextEdit::find(regex);
    } else {
        return QPlainTextEdit::find(text, buildFindFlags(caseSensitive, wholeWord, false));
    }
}

bool CodeEditor::findPrevious(const QString& text, bool caseSensitive, bool wholeWord, bool useRegex)
{
    m_lastSearchText = text;

    if (useRegex) {
        QRegularExpression regex(text);
        if (!caseSensitive) {
            regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        return QPlainTextEdit::find(regex, QTextDocument::FindBackward);
    } else {
        return QPlainTextEdit::find(text, buildFindFlags(caseSensitive, wholeWord, true));
    }
}

int CodeEditor::replaceAll(const QString& searchText, const QString& replaceText,
                           bool caseSensitive, bool wholeWord, bool useRegex)
{
    int count = 0;
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    // Start from beginning
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);

    QTextDocument::FindFlags flags = buildFindFlags(caseSensitive, wholeWord, false);

    while (true) {
        bool found;
        if (useRegex) {
            QRegularExpression regex(searchText);
            if (!caseSensitive) {
                regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
            }
            found = QPlainTextEdit::find(regex);
        } else {
            found = QPlainTextEdit::find(searchText, flags);
        }

        if (!found) break;

        QTextCursor tc = textCursor();
        tc.insertText(replaceText);
        ++count;
    }

    cursor.endEditBlock();
    return count;
}

bool CodeEditor::replaceCurrent(const QString& replaceText)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) {
        return false;
    }

    cursor.insertText(replaceText);
    return true;
}

int CodeEditor::currentLine() const
{
    return textCursor().blockNumber() + 1;
}

int CodeEditor::currentColumn() const
{
    return textCursor().positionInBlock() + 1;
}

void CodeEditor::highlightMatchingBracket()
{
    // This is called from highlightCurrentLine to add bracket matches to extra selections
}

int CodeEditor::findMatchingBracket(int pos, QChar bracket, bool forward) const
{
    static const QString openBrackets = "([{";
    static const QString closeBrackets = ")]}";

    int direction = forward ? 1 : -1;
    QChar matchBracket;

    if (openBrackets.contains(bracket)) {
        matchBracket = closeBrackets[openBrackets.indexOf(bracket)];
    } else if (closeBrackets.contains(bracket)) {
        matchBracket = openBrackets[closeBrackets.indexOf(bracket)];
    } else {
        return -1;
    }

    int depth = 1;
    int currentPos = pos + direction;
    int docLength = document()->characterCount();

    while (currentPos >= 0 && currentPos < docLength && depth > 0) {
        QChar ch = document()->characterAt(currentPos);
        if (ch == bracket) {
            depth++;
        } else if (ch == matchBracket) {
            depth--;
        }
        if (depth == 0) {
            return currentPos;
        }
        currentPos += direction;
    }

    return -1;
}

} // namespace XXMLStudio
