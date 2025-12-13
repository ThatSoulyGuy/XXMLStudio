#ifndef GITFETCHER_H
#define GITFETCHER_H

#include <QObject>
#include <QString>
#include <QProcess>

namespace XXMLStudio {

/**
 * Handles Git operations for fetching dependencies.
 * Uses system git executable via QProcess.
 */
class GitFetcher : public QObject
{
    Q_OBJECT

public:
    explicit GitFetcher(QObject* parent = nullptr);
    ~GitFetcher();

    // Clone a repository to a specific path
    void clone(const QString& url, const QString& targetPath, const QString& tag = QString());

    // Checkout a specific tag or branch
    void checkout(const QString& repoPath, const QString& ref);

    // Get the current commit hash
    QString getCurrentCommit(const QString& repoPath);

    // Check if a path is a valid git repository
    bool isGitRepository(const QString& path);

    // Cancel ongoing operation
    void cancel();

    // Check if an operation is running
    bool isRunning() const;

signals:
    void progress(const QString& message);
    void finished(bool success, const QString& path);
    void error(const QString& message);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStdout();
    void onReadyReadStderr();

private:
    enum class Operation {
        None,
        Clone,
        Checkout
    };

    void startProcess(const QStringList& args);
    QString findGitExecutable();

    QProcess* m_process = nullptr;
    Operation m_currentOperation = Operation::None;
    QString m_targetPath;
    QString m_gitExecutable;
    QString m_currentUrl;
    QString m_lastErrorOutput;
};

} // namespace XXMLStudio

#endif // GITFETCHER_H
