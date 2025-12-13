#include "BuildOutputPanel.h"

#include <QAction>
#include <QScrollBar>
#include <QRegularExpression>

namespace XXMLStudio {

BuildOutputPanel::BuildOutputPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    resetFormat();
}

BuildOutputPanel::~BuildOutputPanel()
{
}

void BuildOutputPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));

    QAction* clearAction = m_toolbar->addAction(tr("Clear"));
    connect(clearAction, &QAction::triggered, this, &BuildOutputPanel::clear);

    QAction* scrollAction = m_toolbar->addAction(tr("Scroll to Bottom"));
    connect(scrollAction, &QAction::triggered, this, &BuildOutputPanel::scrollToBottom);

    m_layout->addWidget(m_toolbar);

    // Output text
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_output->setMaximumBlockCount(10000);  // Limit buffer size

    // Use monospace font
    QFont font("Consolas", 9);
    font.setStyleHint(QFont::Monospace);
    m_output->setFont(font);

    // Dark background
    QPalette pal = m_output->palette();
    m_defaultBg = QColor("#1e1e1e");
    m_defaultFg = QColor("#cccccc");
    pal.setColor(QPalette::Base, m_defaultBg);
    pal.setColor(QPalette::Text, m_defaultFg);
    m_output->setPalette(pal);

    m_layout->addWidget(m_output);
}

void BuildOutputPanel::resetFormat()
{
    m_currentFormat = QTextCharFormat();
    m_currentFormat.setForeground(m_defaultFg);
    m_currentFormat.setBackground(Qt::transparent);
    m_currentFormat.setFontWeight(QFont::Normal);
    m_currentFormat.setFontItalic(false);
    m_currentFormat.setFontUnderline(false);
}

void BuildOutputPanel::clear()
{
    m_output->clear();
    resetFormat();
}

void BuildOutputPanel::appendText(const QString& text)
{
    appendAnsiText(text);
    scrollToBottom();
}

void BuildOutputPanel::appendAnsiText(const QString& text)
{
    // Regex to match ANSI escape sequences: ESC[ ... m
    // ESC can be \x1b or \033
    static QRegularExpression ansiRegex(R"(\x1b\[([0-9;]*)m)");

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);

    int lastEnd = 0;
    QRegularExpressionMatchIterator it = ansiRegex.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        // Insert text before this escape sequence
        if (match.capturedStart() > lastEnd) {
            QString plainText = text.mid(lastEnd, match.capturedStart() - lastEnd);
            cursor.insertText(plainText, m_currentFormat);
        }

        // Parse and apply the escape code
        parseAnsiCode(match.captured(1));

        lastEnd = match.capturedEnd();
    }

    // Insert remaining text after last escape sequence
    if (lastEnd < text.length()) {
        cursor.insertText(text.mid(lastEnd), m_currentFormat);
    }

    m_output->setTextCursor(cursor);
}

void BuildOutputPanel::parseAnsiCode(const QString& code)
{
    // Standard ANSI colors
    static const QColor ansiColors[] = {
        QColor("#000000"),  // 0: Black
        QColor("#cd3131"),  // 1: Red
        QColor("#0dbc79"),  // 2: Green
        QColor("#e5e510"),  // 3: Yellow
        QColor("#2472c8"),  // 4: Blue
        QColor("#bc3fbc"),  // 5: Magenta
        QColor("#11a8cd"),  // 6: Cyan
        QColor("#e5e5e5"),  // 7: White
    };

    // Bright ANSI colors
    static const QColor ansiBrightColors[] = {
        QColor("#666666"),  // 0: Bright Black (Gray)
        QColor("#f14c4c"),  // 1: Bright Red
        QColor("#23d18b"),  // 2: Bright Green
        QColor("#f5f543"),  // 3: Bright Yellow
        QColor("#3b8eea"),  // 4: Bright Blue
        QColor("#d670d6"),  // 5: Bright Magenta
        QColor("#29b8db"),  // 6: Bright Cyan
        QColor("#ffffff"),  // 7: Bright White
    };

    if (code.isEmpty()) {
        resetFormat();
        return;
    }

    QStringList codes = code.split(';');
    bool bright = false;

    for (const QString& c : codes) {
        int n = c.toInt();

        switch (n) {
            case 0:  // Reset
                resetFormat();
                bright = false;
                break;
            case 1:  // Bold/Bright
                m_currentFormat.setFontWeight(QFont::Bold);
                bright = true;
                break;
            case 2:  // Dim
                m_currentFormat.setFontWeight(QFont::Light);
                break;
            case 3:  // Italic
                m_currentFormat.setFontItalic(true);
                break;
            case 4:  // Underline
                m_currentFormat.setFontUnderline(true);
                break;
            case 22: // Normal intensity
                m_currentFormat.setFontWeight(QFont::Normal);
                bright = false;
                break;
            case 23: // Not italic
                m_currentFormat.setFontItalic(false);
                break;
            case 24: // Not underlined
                m_currentFormat.setFontUnderline(false);
                break;
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                // Foreground colors
                if (bright) {
                    m_currentFormat.setForeground(ansiBrightColors[n - 30]);
                } else {
                    m_currentFormat.setForeground(ansiColors[n - 30]);
                }
                break;
            case 39: // Default foreground
                m_currentFormat.setForeground(m_defaultFg);
                break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                // Background colors
                m_currentFormat.setBackground(ansiColors[n - 40]);
                break;
            case 49: // Default background
                m_currentFormat.setBackground(Qt::transparent);
                break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                // Bright foreground colors
                m_currentFormat.setForeground(ansiBrightColors[n - 90]);
                break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                // Bright background colors
                m_currentFormat.setBackground(ansiBrightColors[n - 100]);
                break;
        }
    }
}

void BuildOutputPanel::appendError(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#F44747"));  // Red

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    m_output->setTextCursor(cursor);

    scrollToBottom();
}

void BuildOutputPanel::appendWarning(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#CCA700"));  // Yellow/Orange

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    m_output->setTextCursor(cursor);

    scrollToBottom();
}

void BuildOutputPanel::appendSuccess(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#89D185"));  // Green

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text, format);
    m_output->setTextCursor(cursor);

    scrollToBottom();
}

void BuildOutputPanel::scrollToBottom()
{
    QScrollBar* scrollBar = m_output->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

} // namespace XXMLStudio
