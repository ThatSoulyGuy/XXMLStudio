#include "BuildOutputPanel.h"

#include <QAction>
#include <QTextCharFormat>
#include <QScrollBar>

namespace XXMLStudio {

BuildOutputPanel::BuildOutputPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
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

    m_layout->addWidget(m_output);
}

void BuildOutputPanel::clear()
{
    m_output->clear();
}

void BuildOutputPanel::appendText(const QString& text)
{
    m_output->appendPlainText(text);
    scrollToBottom();
}

void BuildOutputPanel::appendError(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#F44747"));  // Red

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);

    scrollToBottom();
}

void BuildOutputPanel::appendWarning(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#CCA700"));  // Yellow/Orange

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);

    scrollToBottom();
}

void BuildOutputPanel::appendSuccess(const QString& text)
{
    QTextCharFormat format;
    format.setForeground(QColor("#89D185"));  // Green

    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n", format);

    scrollToBottom();
}

void BuildOutputPanel::scrollToBottom()
{
    QScrollBar* scrollBar = m_output->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

} // namespace XXMLStudio
