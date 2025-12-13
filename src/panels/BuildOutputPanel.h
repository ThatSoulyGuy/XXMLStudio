#ifndef BUILDOUTPUTPANEL_H
#define BUILDOUTPUTPANEL_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QToolBar>
#include <QTextCharFormat>

namespace XXMLStudio {

/**
 * Panel displaying build output from the compiler.
 * Supports ANSI escape codes for colored output.
 */
class BuildOutputPanel : public QWidget
{
    Q_OBJECT

public:
    explicit BuildOutputPanel(QWidget* parent = nullptr);
    ~BuildOutputPanel();

    void clear();
    void appendText(const QString& text);
    void appendError(const QString& text);
    void appendWarning(const QString& text);
    void appendSuccess(const QString& text);

public slots:
    void scrollToBottom();

signals:
    void lineClicked(const QString& file, int line);

private:
    void setupUi();
    void appendAnsiText(const QString& text);
    void parseAnsiCode(const QString& code);
    void resetFormat();

    QVBoxLayout* m_layout = nullptr;
    QToolBar* m_toolbar = nullptr;
    QPlainTextEdit* m_output = nullptr;

    // Current text format for ANSI rendering
    QTextCharFormat m_currentFormat;
    QColor m_defaultFg;
    QColor m_defaultBg;
};

} // namespace XXMLStudio

#endif // BUILDOUTPUTPANEL_H
