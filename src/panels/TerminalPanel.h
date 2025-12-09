#ifndef TERMINALPANEL_H
#define TERMINALPANEL_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QProcess>

namespace XXMLStudio {

/**
 * Integrated terminal panel for running commands.
 */
class TerminalPanel : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel();

    void setWorkingDirectory(const QString& path);
    void runCommand(const QString& command, const QStringList& arguments = {});
    void runExecutable(const QString& path, const QStringList& arguments = {});
    void terminate();
    void clear();
    void appendText(const QString& text);

    bool isRunning() const;

signals:
    void processStarted();
    void processFinished(int exitCode);
    void outputReceived(const QString& text);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUi();
    void appendOutput(const QString& text, const QColor& color = QColor());

    QVBoxLayout* m_layout = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QProcess* m_process = nullptr;
    QString m_workingDirectory;
};

} // namespace XXMLStudio

#endif // TERMINALPANEL_H
