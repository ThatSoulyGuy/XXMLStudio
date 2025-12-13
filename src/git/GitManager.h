#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QTimer>
#include <QHash>
#include "GitTypes.h"

namespace XXMLStudio {

/**
 * Central Git operations manager.
 * Handles all Git commands via QProcess with async signals.
 * Automatically watches for file changes and refreshes status.
 */
class GitManager : public QObject
{
    Q_OBJECT

public:
    explicit GitManager(QObject* parent = nullptr);
    ~GitManager();

    // Repository management
    void setRepositoryPath(const QString& path);
    QString repositoryPath() const { return m_repoPath; }
    bool isGitRepository() const { return m_isGitRepo; }
    void initRepository();

    // Status operations
    void refreshStatus();
    GitRepositoryStatus cachedStatus() const { return m_cachedStatus; }

    // Staging operations
    void stage(const QStringList& paths);
    void unstage(const QStringList& paths);
    void stageAll();
    void unstageAll();
    void discardChanges(const QStringList& paths);

    // Commit operations
    void commit(const QString& message);
    void commitAmend(const QString& message);

    // Remote operations
    void fetch(const QString& remote = "origin");
    void pull(const QString& remote = "origin", const QString& branch = QString());
    void push(const QString& remote = "origin", const QString& branch = QString());
    void pushWithUpstream(const QString& remote, const QString& branch);

    // Remote management
    void getRemotes();
    void addRemote(const QString& name, const QString& url);
    void removeRemote(const QString& name);

    // Branch operations
    void getBranches();
    void checkoutBranch(const QString& branch);
    void createBranch(const QString& name, bool checkout = true);
    void deleteBranch(const QString& name, bool force = false);

    // History operations
    void getLog(int maxCount = 100, const QString& path = QString());

    // Diff operations
    void getDiff(const QString& path = QString(), bool staged = false);

    // File status for ProjectExplorer decorations
    GitFileStatus fileIndexStatus(const QString& path) const;
    GitFileStatus fileWorkTreeStatus(const QString& path) const;

    // Auto-refresh control
    void setAutoRefresh(bool enabled, int intervalMs = 3000);
    bool isAutoRefreshEnabled() const { return m_autoRefreshEnabled; }

    // Check if an operation is running
    bool isBusy() const { return m_currentOperation != Operation::None; }

signals:
    // Status signals
    void repositoryChanged(bool isGitRepo);
    void statusRefreshed(const GitRepositoryStatus& status);
    void initCompleted(bool success, const QString& error);

    // Operation completion signals
    void stageCompleted(bool success, const QString& error);
    void unstageCompleted(bool success, const QString& error);
    void discardCompleted(bool success, const QString& error);
    void commitCompleted(bool success, const QString& commitHash, const QString& error);
    void fetchCompleted(bool success, const QString& error);
    void pullCompleted(bool success, const QString& error);
    void pushCompleted(bool success, const QString& error);
    void pushNeedsUpstream(const QString& currentBranch);  // Emitted when push fails due to no upstream

    // Remote signals
    void remotesReceived(const QStringList& remotes);
    void remoteAdded(bool success, const QString& name, const QString& error);
    void remoteRemoved(bool success, const QString& name, const QString& error);

    // Branch signals
    void branchesReceived(const QList<GitBranch>& branches);
    void branchCheckoutCompleted(bool success, const QString& branch, const QString& error);
    void branchCreated(bool success, const QString& branch, const QString& error);
    void branchDeleted(bool success, const QString& branch, const QString& error);

    // History signals
    void logReceived(const QList<GitCommit>& commits);

    // Diff signals
    void diffReceived(const QString& diff);

    // General signals
    void operationStarted(const QString& operation);
    void operationProgress(const QString& message);
    void operationError(const QString& error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onAutoRefreshTimer();
    void onOperationTimeout();

private:
    enum class Operation {
        None,
        Init,
        Status,
        Stage,
        Unstage,
        Discard,
        Commit,
        Fetch,
        Pull,
        Push,
        Branches,
        Checkout,
        CreateBranch,
        DeleteBranch,
        Log,
        Diff,
        GetRemotes,
        AddRemote,
        RemoveRemote
    };

    struct QueuedCommand {
        Operation operation;
        QStringList args;
        QVariant userData;
    };

    void executeCommand(const QStringList& args, Operation operation, const QVariant& userData = QVariant());
    void queueCommand(const QStringList& args, Operation operation, const QVariant& userData = QVariant());
    void processQueue();
    void handleOperationResult(Operation op, int exitCode, const QString& output, const QString& errorOutput);

    QString findGitExecutable();
    bool detectGitRepository(const QString& path);

    // Parsing methods
    GitRepositoryStatus parseStatus(const QString& output);
    QList<GitBranch> parseBranches(const QString& output);
    QList<GitCommit> parseLog(const QString& output);
    GitFileStatus parseStatusChar(QChar c);

    QString m_gitExecutable;
    QString m_repoPath;
    bool m_isGitRepo = false;

    QProcess* m_process = nullptr;
    Operation m_currentOperation = Operation::None;
    QVariant m_currentUserData;
    QString m_currentOutput;
    QString m_currentErrorOutput;
    QQueue<QueuedCommand> m_commandQueue;

    GitRepositoryStatus m_cachedStatus;
    QHash<QString, GitStatusEntry> m_fileStatusCache;

    QTimer* m_autoRefreshTimer = nullptr;
    bool m_autoRefreshEnabled = true;
    int m_autoRefreshInterval = 3000;

    QTimer* m_operationTimeout = nullptr;
    static const int OPERATION_TIMEOUT_MS = 30000;  // 30 seconds for network operations
};

} // namespace XXMLStudio

#endif // GITMANAGER_H
