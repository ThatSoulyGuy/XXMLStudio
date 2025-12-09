#include "TerminalPanel.h"

#include <QScrollBar>
#include <QTextCharFormat>

namespace XXMLStudio {

TerminalPanel::TerminalPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

TerminalPanel::~TerminalPanel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

void TerminalPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    // Output display
    m_output = new QPlainTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_output->setMaximumBlockCount(10000);

    // Use monospace font
    QFont font("Consolas", 9);
    font.setStyleHint(QFont::Monospace);
    m_output->setFont(font);

    m_layout->addWidget(m_output);

    // Create process
    m_process = new QProcess(this);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &TerminalPanel::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &TerminalPanel::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalPanel::onProcessFinished);
}

void TerminalPanel::setWorkingDirectory(const QString& path)
{
    m_workingDirectory = path;
    m_process->setWorkingDirectory(path);
}

void TerminalPanel::runCommand(const QString& command, const QStringList& arguments)
{
    if (isRunning()) {
        appendOutput(tr("Process already running. Please wait or terminate.\n"),
                    QColor("#F44747"));
        return;
    }

    m_output->clear();
    appendOutput(QString("> %1 %2\n").arg(command, arguments.join(' ')),
                QColor("#569CD6"));

    if (!m_workingDirectory.isEmpty()) {
        m_process->setWorkingDirectory(m_workingDirectory);
    }

    m_process->start(command, arguments);
    emit processStarted();
}

void TerminalPanel::runExecutable(const QString& path, const QStringList& arguments)
{
    runCommand(path, arguments);
}

void TerminalPanel::terminate()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
        if (!m_process->waitForFinished(3000)) {
            m_process->kill();
        }
    }
}

bool TerminalPanel::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void TerminalPanel::clear()
{
    m_output->clear();
}

void TerminalPanel::appendText(const QString& text)
{
    appendOutput(text);
}

void TerminalPanel::onReadyReadStandardOutput()
{
    QString output = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    appendOutput(output);
    emit outputReceived(output);
}

void TerminalPanel::onReadyReadStandardError()
{
    QString output = QString::fromLocal8Bit(m_process->readAllStandardError());
    appendOutput(output, QColor("#F44747"));
    emit outputReceived(output);
}

void TerminalPanel::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString message;
    QColor color;

    if (exitStatus == QProcess::CrashExit) {
        message = tr("\nProcess crashed.\n");
        color = QColor("#F44747");
    } else if (exitCode == 0) {
        message = tr("\nProcess finished successfully.\n");
        color = QColor("#89D185");
    } else {
        message = tr("\nProcess finished with exit code %1.\n").arg(exitCode);
        color = QColor("#CCA700");
    }

    appendOutput(message, color);
    emit processFinished(exitCode);
}

void TerminalPanel::appendOutput(const QString& text, const QColor& color)
{
    QTextCursor cursor = m_output->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (color.isValid()) {
        QTextCharFormat format;
        format.setForeground(color);
        cursor.insertText(text, format);
    } else {
        cursor.insertText(text);
    }

    // Scroll to bottom
    QScrollBar* scrollBar = m_output->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

} // namespace XXMLStudio
