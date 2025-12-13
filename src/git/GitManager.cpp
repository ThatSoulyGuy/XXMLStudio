#include "GitManager.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

namespace XXMLStudio {

GitManager::GitManager(QObject* parent)
    : QObject(parent)
{
    m_gitExecutable = findGitExecutable();
    qDebug() << "[GitManager] Initialized with git executable:" << m_gitExecutable;

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &GitManager::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &GitManager::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &GitManager::onReadyReadStderr);

    // Set environment to prevent git from prompting for credentials
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_TERMINAL_PROMPT", "0");  // Disable terminal prompts
    env.insert("GIT_ASKPASS", "");           // Disable askpass
    env.insert("SSH_ASKPASS", "");           // Disable SSH askpass
    m_process->setProcessEnvironment(env);

    m_autoRefreshTimer = new QTimer(this);
    connect(m_autoRefreshTimer, &QTimer::timeout,
            this, &GitManager::onAutoRefreshTimer);

    // Operation timeout timer
    m_operationTimeout = new QTimer(this);
    m_operationTimeout->setSingleShot(true);
    connect(m_operationTimeout, &QTimer::timeout,
            this, &GitManager::onOperationTimeout);
}

GitManager::~GitManager()
{
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

QString GitManager::findGitExecutable()
{
    // Try to find git in PATH
    QString git = QStandardPaths::findExecutable("git");
    if (!git.isEmpty()) {
        return git;
    }

    // Windows-specific paths
#ifdef Q_OS_WIN
    QStringList windowsPaths = {
        "C:/Program Files/Git/bin/git.exe",
        "C:/Program Files (x86)/Git/bin/git.exe",
        "C:/Git/bin/git.exe"
    };
    for (const QString& path : windowsPaths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }
#endif

    // Fallback - hope it's in PATH
    return "git";
}

bool GitManager::detectGitRepository(const QString& path)
{
    QDir dir(path);
    while (dir.exists()) {
        if (dir.exists(".git")) {
            return true;
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return false;
}

void GitManager::setRepositoryPath(const QString& path)
{
    qDebug() << "[GitManager] setRepositoryPath called with:" << path;

    if (m_repoPath == path) {
        qDebug() << "[GitManager] Same path, skipping";
        return;
    }

    m_repoPath = path;
    m_cachedStatus = GitRepositoryStatus();
    m_fileStatusCache.clear();

    bool wasGitRepo = m_isGitRepo;
    m_isGitRepo = !path.isEmpty() && detectGitRepository(path);

    qDebug() << "[GitManager] Is Git repo:" << m_isGitRepo << "(was:" << wasGitRepo << ")";

    if (m_isGitRepo != wasGitRepo) {
        qDebug() << "[GitManager] Emitting repositoryChanged:" << m_isGitRepo;
        emit repositoryChanged(m_isGitRepo);
    }

    if (m_isGitRepo) {
        // Start auto-refresh if enabled
        if (m_autoRefreshEnabled) {
            qDebug() << "[GitManager] Starting auto-refresh timer:" << m_autoRefreshInterval << "ms";
            m_autoRefreshTimer->start(m_autoRefreshInterval);
        }
        // Initial status refresh
        qDebug() << "[GitManager] Calling initial refreshStatus()";
        refreshStatus();
    } else {
        m_autoRefreshTimer->stop();
    }
}

void GitManager::initRepository()
{
    if (m_repoPath.isEmpty()) {
        emit initCompleted(false, tr("No directory path set"));
        return;
    }

    if (m_isGitRepo) {
        emit initCompleted(false, tr("Directory is already a Git repository"));
        return;
    }

    QStringList args = {"init"};
    emit operationStarted(tr("Initializing Git repository..."));
    executeCommand(args, Operation::Init);
}

void GitManager::setAutoRefresh(bool enabled, int intervalMs)
{
    m_autoRefreshEnabled = enabled;
    m_autoRefreshInterval = intervalMs;

    if (enabled && m_isGitRepo) {
        m_autoRefreshTimer->start(intervalMs);
    } else {
        m_autoRefreshTimer->stop();
    }
}

void GitManager::onAutoRefreshTimer()
{
    if (m_isGitRepo && m_currentOperation == Operation::None) {
        refreshStatus();
    }
}

void GitManager::executeCommand(const QStringList& args, Operation operation, const QVariant& userData)
{
    qDebug() << "[GitManager] executeCommand: operation=" << static_cast<int>(operation)
             << "args=" << args << "path=" << m_repoPath;

    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "[GitManager] Process busy (state=" << m_process->state() << "), queueing command";
        queueCommand(args, operation, userData);
        return;
    }

    m_currentOperation = operation;
    m_currentUserData = userData;
    m_currentOutput.clear();
    m_currentErrorOutput.clear();

    m_process->setWorkingDirectory(m_repoPath);
    qDebug() << "[GitManager] Starting process:" << m_gitExecutable << args;
    m_process->start(m_gitExecutable, args);

    if (!m_process->waitForStarted(1000)) {
        qDebug() << "[GitManager] ERROR: Failed to start process:" << m_process->errorString();
        emit operationError(tr("Failed to start git: %1").arg(m_process->errorString()));
        m_currentOperation = Operation::None;
    } else {
        qDebug() << "[GitManager] Process started successfully, PID:" << m_process->processId();
        // Start timeout timer for network operations
        if (operation == Operation::Push || operation == Operation::Pull ||
            operation == Operation::Fetch || operation == Operation::AddRemote) {
            m_operationTimeout->start(OPERATION_TIMEOUT_MS);
        }
    }
}

void GitManager::queueCommand(const QStringList& args, Operation operation, const QVariant& userData)
{
    QueuedCommand cmd;
    cmd.operation = operation;
    cmd.args = args;
    cmd.userData = userData;
    m_commandQueue.enqueue(cmd);
}

void GitManager::processQueue()
{
    if (m_commandQueue.isEmpty()) {
        qDebug() << "[GitManager] processQueue: queue is empty";
        return;
    }

    qDebug() << "[GitManager] processQueue: processing next command, queue size=" << m_commandQueue.size();
    QueuedCommand cmd = m_commandQueue.dequeue();
    executeCommand(cmd.args, cmd.operation, cmd.userData);
}

void GitManager::onReadyReadStdout()
{
    m_currentOutput += QString::fromUtf8(m_process->readAllStandardOutput());
}

void GitManager::onReadyReadStderr()
{
    m_currentErrorOutput += QString::fromUtf8(m_process->readAllStandardError());
}

void GitManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    // Stop timeout timer
    m_operationTimeout->stop();

    bool success = (status == QProcess::NormalExit && exitCode == 0);
    qDebug() << "[GitManager] onProcessFinished: operation=" << static_cast<int>(m_currentOperation)
             << "exitCode=" << exitCode << "exitStatus=" << status << "success=" << success;
    qDebug() << "[GitManager] stdout:" << m_currentOutput.left(500);
    if (!m_currentErrorOutput.isEmpty()) {
        qDebug() << "[GitManager] stderr:" << m_currentErrorOutput;
    }

    Operation completedOp = m_currentOperation;
    handleOperationResult(completedOp, exitCode, m_currentOutput, m_currentErrorOutput);

    qDebug() << "[GitManager] Operation" << static_cast<int>(completedOp) << "completed, queue size:" << m_commandQueue.size();
    m_currentOperation = Operation::None;
    m_currentUserData = QVariant();

    processQueue();
}

void GitManager::onOperationTimeout()
{
    qDebug() << "[GitManager] TIMEOUT: Operation" << static_cast<int>(m_currentOperation) << "timed out after" << OPERATION_TIMEOUT_MS << "ms";

    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }

    Operation timedOutOp = m_currentOperation;
    QString errorMsg = tr("Operation timed out. This may indicate:\n"
                          "- Network connectivity issues\n"
                          "- Authentication required (set up SSH keys or credential helper)\n"
                          "- Invalid remote URL");

    // Emit appropriate error signal based on operation
    switch (timedOutOp) {
    case Operation::Push:
        emit pushCompleted(false, errorMsg);
        break;
    case Operation::Pull:
        emit pullCompleted(false, errorMsg);
        break;
    case Operation::Fetch:
        emit fetchCompleted(false, errorMsg);
        break;
    case Operation::AddRemote:
        emit remoteAdded(false, m_currentUserData.toString(), errorMsg);
        break;
    default:
        emit operationError(errorMsg);
        break;
    }

    m_currentOperation = Operation::None;
    m_currentUserData = QVariant();
    processQueue();
}

void GitManager::onProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
    case QProcess::FailedToStart:
        errorMessage = tr("Git failed to start. Please ensure Git is installed.");
        break;
    case QProcess::Crashed:
        errorMessage = tr("Git process crashed.");
        break;
    case QProcess::Timedout:
        errorMessage = tr("Git operation timed out.");
        break;
    case QProcess::WriteError:
        errorMessage = tr("Error writing to Git process.");
        break;
    case QProcess::ReadError:
        errorMessage = tr("Error reading from Git process.");
        break;
    default:
        errorMessage = tr("Unknown Git error occurred.");
        break;
    }

    emit operationError(errorMessage);
    m_currentOperation = Operation::None;
    processQueue();
}

void GitManager::handleOperationResult(Operation op, int exitCode, const QString& output, const QString& errorOutput)
{
    bool success = (exitCode == 0);

    switch (op) {
    case Operation::Init: {
        emit initCompleted(success, success ? QString() : errorOutput);
        if (success) {
            // Re-detect and set up the newly created repository
            m_isGitRepo = detectGitRepository(m_repoPath);
            qDebug() << "[GitManager] Init successful, isGitRepo:" << m_isGitRepo;
            emit repositoryChanged(m_isGitRepo);
            if (m_isGitRepo) {
                if (m_autoRefreshEnabled) {
                    m_autoRefreshTimer->start(m_autoRefreshInterval);
                }
                // Queue all refresh operations to update the UI
                refreshStatus();
                getBranches();
                getRemotes();
            }
        }
        break;
    }
    case Operation::Status: {
        if (success) {
            m_cachedStatus = parseStatus(output);
            qDebug() << "[GitManager] Parsed status - branch:" << m_cachedStatus.branch
                     << "entries:" << m_cachedStatus.entries.size()
                     << "ahead:" << m_cachedStatus.aheadCount
                     << "behind:" << m_cachedStatus.behindCount;
            // Update file status cache
            m_fileStatusCache.clear();
            for (const GitStatusEntry& entry : m_cachedStatus.entries) {
                m_fileStatusCache[entry.path] = entry;
                qDebug() << "[GitManager] File entry:" << entry.path
                         << "index:" << static_cast<int>(entry.indexStatus)
                         << "worktree:" << static_cast<int>(entry.workTreeStatus);
            }
            qDebug() << "[GitManager] Emitting statusRefreshed signal";
            emit statusRefreshed(m_cachedStatus);
        } else {
            qDebug() << "[GitManager] Status operation failed:" << errorOutput;
            emit operationError(tr("Failed to get status: %1").arg(errorOutput));
        }
        break;
    }
    case Operation::Stage:
        emit stageCompleted(success, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;

    case Operation::Unstage:
        emit unstageCompleted(success, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;

    case Operation::Discard:
        emit discardCompleted(success, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;

    case Operation::Commit: {
        QString commitHash;
        if (success) {
            // Extract commit hash from output
            QRegularExpression re("\\[\\w+\\s+([a-f0-9]+)\\]");
            QRegularExpressionMatch match = re.match(output);
            if (match.hasMatch()) {
                commitHash = match.captured(1);
            }
        }
        emit commitCompleted(success, commitHash, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;
    }
    case Operation::Fetch:
        emit fetchCompleted(success, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;

    case Operation::Pull:
        emit pullCompleted(success, success ? QString() : errorOutput);
        if (success) refreshStatus();
        break;

    case Operation::Push:
        qDebug() << "[GitManager] Push operation completed - success:" << success;
        qDebug() << "[GitManager] Push errorOutput:" << errorOutput;
        if (!success) {
            // Check for various "no remote/upstream" errors
            bool needsUpstream = false;

            // No upstream branch configured
            if (errorOutput.contains("has no upstream branch") ||
                errorOutput.contains("no upstream configured") ||
                (errorOutput.contains("The current branch") && errorOutput.contains("has no upstream"))) {
                needsUpstream = true;
            }
            // No remote configured at all
            else if (errorOutput.contains("does not appear to be a git repository") ||
                     errorOutput.contains("No configured push destination") ||
                     errorOutput.contains("Could not read from remote repository") ||
                     errorOutput.contains("fatal: 'origin'")) {
                needsUpstream = true;
            }

            if (needsUpstream) {
                qDebug() << "[GitManager] Push failed - needs upstream/remote configuration";
                emit pushNeedsUpstream(m_cachedStatus.branch);
            } else {
                qDebug() << "[GitManager] Push failed with other error";
                emit pushCompleted(false, errorOutput);
            }
        } else {
            emit pushCompleted(true, QString());
            refreshStatus();
        }
        break;

    case Operation::Branches: {
        if (success) {
            QList<GitBranch> branches = parseBranches(output);
            emit branchesReceived(branches);
        } else {
            emit operationError(tr("Failed to get branches: %1").arg(errorOutput));
        }
        break;
    }
    case Operation::Checkout: {
        QString branch = m_currentUserData.toString();
        emit branchCheckoutCompleted(success, branch, success ? QString() : errorOutput);
        if (success) {
            refreshStatus();
            getBranches();
        }
        break;
    }
    case Operation::CreateBranch: {
        QString branch = m_currentUserData.toString();
        emit branchCreated(success, branch, success ? QString() : errorOutput);
        if (success) getBranches();
        break;
    }
    case Operation::DeleteBranch: {
        QString branch = m_currentUserData.toString();
        emit branchDeleted(success, branch, success ? QString() : errorOutput);
        if (success) getBranches();
        break;
    }
    case Operation::Log: {
        if (success) {
            QList<GitCommit> commits = parseLog(output);
            emit logReceived(commits);
        } else {
            emit operationError(tr("Failed to get log: %1").arg(errorOutput));
        }
        break;
    }
    case Operation::Diff:
        emit diffReceived(success ? output : QString());
        break;

    case Operation::GetRemotes: {
        if (success) {
            QStringList remotes = output.split('\n', Qt::SkipEmptyParts);
            qDebug() << "[GitManager] Remotes:" << remotes;
            emit remotesReceived(remotes);
        } else {
            emit operationError(tr("Failed to get remotes: %1").arg(errorOutput));
        }
        break;
    }
    case Operation::AddRemote: {
        QString remoteName = m_currentUserData.toString();
        qDebug() << "[GitManager] Add remote result: success=" << success << "remoteName=" << remoteName << "error=" << errorOutput;
        emit operationProgress(tr("Remote %1 added").arg(remoteName));
        emit remoteAdded(success, remoteName, success ? QString() : errorOutput);
        // Note: Don't call getRemotes() here - let the caller decide next steps
        // The remoteAdded signal handler should proceed with push/pull
        break;
    }
    case Operation::RemoveRemote: {
        QString remoteName = m_currentUserData.toString();
        emit remoteRemoved(success, remoteName, success ? QString() : errorOutput);
        if (success) getRemotes();
        break;
    }
    default:
        break;
    }
}

// ============================================================================
// Status Operations
// ============================================================================

void GitManager::refreshStatus()
{
    if (!m_isGitRepo) {
        return;
    }

    // Use porcelain v2 format for detailed status
    QStringList args = {"status", "--porcelain=v2", "--branch", "--untracked-files=all"};
    executeCommand(args, Operation::Status);
}

GitRepositoryStatus GitManager::parseStatus(const QString& output)
{
    GitRepositoryStatus status;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        if (line.startsWith("# branch.head ")) {
            status.branch = line.mid(14);
            if (status.branch == "(detached)") {
                status.detachedHead = true;
            }
        } else if (line.startsWith("# branch.upstream ")) {
            status.upstream = line.mid(18);
        } else if (line.startsWith("# branch.ab ")) {
            // Parse ahead/behind: # branch.ab +1 -2
            QRegularExpression re("\\+(-?\\d+)\\s+(-?\\d+)");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                status.aheadCount = match.captured(1).toInt();
                status.behindCount = -match.captured(2).toInt();
            }
        } else if (line.startsWith("1 ") || line.startsWith("2 ")) {
            // Ordinary or renamed/copied entry
            // Format: 1 XY sub mH mI mW hH hI path
            // or:     2 XY sub mH mI mW hH hI X path\torigPath
            GitStatusEntry entry;
            QStringList parts = line.split(' ');
            if (parts.size() >= 9) {
                QString xy = parts[1];
                entry.indexStatus = parseStatusChar(xy[0]);
                entry.workTreeStatus = parseStatusChar(xy[1]);

                if (line.startsWith("2 ")) {
                    // Renamed/copied - path contains tab separator
                    QString pathPart = parts.mid(8).join(' ');
                    int tabIndex = pathPart.indexOf('\t');
                    if (tabIndex != -1) {
                        entry.path = pathPart.left(tabIndex);
                        entry.oldPath = pathPart.mid(tabIndex + 1);
                    } else {
                        entry.path = pathPart;
                    }
                } else {
                    entry.path = parts.mid(8).join(' ');
                }
                status.entries.append(entry);
            }
        } else if (line.startsWith("u ")) {
            // Unmerged entry
            GitStatusEntry entry;
            QStringList parts = line.split(' ');
            if (parts.size() >= 11) {
                entry.indexStatus = GitFileStatus::Conflicted;
                entry.workTreeStatus = GitFileStatus::Conflicted;
                entry.path = parts.mid(10).join(' ');
                status.entries.append(entry);
            }
        } else if (line.startsWith("? ")) {
            // Untracked
            GitStatusEntry entry;
            entry.indexStatus = GitFileStatus::Untracked;
            entry.workTreeStatus = GitFileStatus::Untracked;
            entry.path = line.mid(2);
            status.entries.append(entry);
        } else if (line.startsWith("! ")) {
            // Ignored
            GitStatusEntry entry;
            entry.indexStatus = GitFileStatus::Ignored;
            entry.workTreeStatus = GitFileStatus::Ignored;
            entry.path = line.mid(2);
            status.entries.append(entry);
        }
    }

    return status;
}

GitFileStatus GitManager::parseStatusChar(QChar c)
{
    switch (c.toLatin1()) {
    case 'M': return GitFileStatus::Modified;
    case 'T': return GitFileStatus::TypeChanged;
    case 'A': return GitFileStatus::Added;
    case 'D': return GitFileStatus::Deleted;
    case 'R': return GitFileStatus::Renamed;
    case 'C': return GitFileStatus::Copied;
    case 'U': return GitFileStatus::Conflicted;
    case '?': return GitFileStatus::Untracked;
    case '!': return GitFileStatus::Ignored;
    default: return GitFileStatus::Unmodified;
    }
}

GitFileStatus GitManager::fileIndexStatus(const QString& path) const
{
    if (m_fileStatusCache.contains(path)) {
        return m_fileStatusCache[path].indexStatus;
    }
    return GitFileStatus::Unmodified;
}

GitFileStatus GitManager::fileWorkTreeStatus(const QString& path) const
{
    if (m_fileStatusCache.contains(path)) {
        return m_fileStatusCache[path].workTreeStatus;
    }
    return GitFileStatus::Unmodified;
}

// ============================================================================
// Staging Operations
// ============================================================================

void GitManager::stage(const QStringList& paths)
{
    if (!m_isGitRepo || paths.isEmpty()) {
        return;
    }

    QStringList args = {"add", "--"};
    args.append(paths);
    emit operationStarted(tr("Staging files..."));
    executeCommand(args, Operation::Stage);
}

void GitManager::unstage(const QStringList& paths)
{
    if (!m_isGitRepo || paths.isEmpty()) {
        return;
    }

    QStringList args = {"reset", "HEAD", "--"};
    args.append(paths);
    emit operationStarted(tr("Unstaging files..."));
    executeCommand(args, Operation::Unstage);
}

void GitManager::stageAll()
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"add", "-A"};
    emit operationStarted(tr("Staging all files..."));
    executeCommand(args, Operation::Stage);
}

void GitManager::unstageAll()
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"reset", "HEAD"};
    emit operationStarted(tr("Unstaging all files..."));
    executeCommand(args, Operation::Unstage);
}

void GitManager::discardChanges(const QStringList& paths)
{
    if (!m_isGitRepo || paths.isEmpty()) {
        return;
    }

    QStringList args = {"checkout", "--"};
    args.append(paths);
    emit operationStarted(tr("Discarding changes..."));
    executeCommand(args, Operation::Discard);
}

// ============================================================================
// Commit Operations
// ============================================================================

void GitManager::commit(const QString& message)
{
    if (!m_isGitRepo || message.isEmpty()) {
        return;
    }

    QStringList args = {"commit", "-m", message};
    emit operationStarted(tr("Committing..."));
    executeCommand(args, Operation::Commit);
}

void GitManager::commitAmend(const QString& message)
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"commit", "--amend"};
    if (!message.isEmpty()) {
        args << "-m" << message;
    } else {
        args << "--no-edit";
    }
    emit operationStarted(tr("Amending commit..."));
    executeCommand(args, Operation::Commit);
}

// ============================================================================
// Remote Operations
// ============================================================================

void GitManager::fetch(const QString& remote)
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"fetch", remote};
    emit operationStarted(tr("Fetching from %1...").arg(remote));
    executeCommand(args, Operation::Fetch);
}

void GitManager::pull(const QString& remote, const QString& branch)
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"pull", remote};
    if (!branch.isEmpty()) {
        args << branch;
    }
    emit operationStarted(tr("Pulling from %1...").arg(remote));
    executeCommand(args, Operation::Pull);
}

void GitManager::push(const QString& remote, const QString& branch)
{
    qDebug() << "[GitManager] push() called - remote:" << remote << "branch:" << branch << "isGitRepo:" << m_isGitRepo;

    if (!m_isGitRepo) {
        qDebug() << "[GitManager] Not a git repo, returning";
        return;
    }

    QStringList args = {"push", remote};
    if (!branch.isEmpty()) {
        args << branch;
    }
    emit operationStarted(tr("Pushing to %1...").arg(remote));
    executeCommand(args, Operation::Push);
}

void GitManager::pushWithUpstream(const QString& remote, const QString& branch)
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"push", "-u", remote, branch};
    emit operationStarted(tr("Pushing to %1/%2...").arg(remote, branch));
    executeCommand(args, Operation::Push);
}

// ============================================================================
// Remote Management
// ============================================================================

void GitManager::getRemotes()
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"remote"};
    executeCommand(args, Operation::GetRemotes);
}

void GitManager::addRemote(const QString& name, const QString& url)
{
    qDebug() << "[GitManager] >>> addRemote() called - name=" << name << "url=" << url << "isGitRepo=" << m_isGitRepo;

    if (!m_isGitRepo) {
        qDebug() << "[GitManager] addRemote() - not a git repo, returning";
        emit remoteAdded(false, name, tr("Not a git repository"));
        return;
    }
    if (name.isEmpty()) {
        qDebug() << "[GitManager] addRemote() - name is empty, returning";
        emit remoteAdded(false, name, tr("Remote name cannot be empty"));
        return;
    }
    if (url.isEmpty()) {
        qDebug() << "[GitManager] addRemote() - url is empty, returning";
        emit remoteAdded(false, name, tr("Remote URL cannot be empty"));
        return;
    }

    QStringList args = {"remote", "add", name, url};
    qDebug() << "[GitManager] Emitting operationStarted for addRemote";
    emit operationStarted(tr("Adding remote %1...").arg(name));
    qDebug() << "[GitManager] Calling executeCommand for addRemote";
    executeCommand(args, Operation::AddRemote, name);
    qDebug() << "[GitManager] <<< addRemote() returned";
}

void GitManager::removeRemote(const QString& name)
{
    if (!m_isGitRepo || name.isEmpty()) {
        return;
    }

    QStringList args = {"remote", "remove", name};
    emit operationStarted(tr("Removing remote %1...").arg(name));
    executeCommand(args, Operation::RemoveRemote, name);
}

// ============================================================================
// Branch Operations
// ============================================================================

void GitManager::getBranches()
{
    if (!m_isGitRepo) {
        return;
    }

    // Get all branches with upstream info
    QStringList args = {"branch", "-a", "-vv"};
    executeCommand(args, Operation::Branches);
}

QList<GitBranch> GitManager::parseBranches(const QString& output)
{
    QList<GitBranch> branches;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        GitBranch branch;

        // Check if current branch
        bool isCurrent = line.startsWith("* ");
        QString trimmedLine = line.mid(2).trimmed();

        // Parse branch name and other info
        // Format: name hash [upstream: ahead N, behind M] subject
        // or:     remotes/origin/name hash subject

        QStringList parts = trimmedLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) continue;

        branch.name = parts[0];
        branch.isCurrent = isCurrent;

        // Check if remote branch
        if (branch.name.startsWith("remotes/")) {
            branch.isRemote = true;
            branch.name = branch.name.mid(8); // Remove "remotes/" prefix
        }

        // Skip HEAD pointer
        if (branch.name == "HEAD" || branch.name.contains("->")) {
            continue;
        }

        if (parts.size() >= 2) {
            branch.lastCommitHash = parts[1];
        }

        // Try to find upstream info in brackets
        int bracketStart = trimmedLine.indexOf('[');
        int bracketEnd = trimmedLine.indexOf(']');
        if (bracketStart != -1 && bracketEnd != -1) {
            QString upstreamInfo = trimmedLine.mid(bracketStart + 1, bracketEnd - bracketStart - 1);

            // Extract upstream branch
            int colonPos = upstreamInfo.indexOf(':');
            if (colonPos != -1) {
                branch.upstream = upstreamInfo.left(colonPos).trimmed();
                // Parse ahead/behind
                QString abInfo = upstreamInfo.mid(colonPos + 1);
                QRegularExpression aheadRe("ahead (\\d+)");
                QRegularExpression behindRe("behind (\\d+)");
                QRegularExpressionMatch aheadMatch = aheadRe.match(abInfo);
                QRegularExpressionMatch behindMatch = behindRe.match(abInfo);
                if (aheadMatch.hasMatch()) {
                    branch.aheadCount = aheadMatch.captured(1).toInt();
                }
                if (behindMatch.hasMatch()) {
                    branch.behindCount = behindMatch.captured(1).toInt();
                }
            } else {
                branch.upstream = upstreamInfo.trimmed();
            }
        }

        // Subject is everything after the bracket or hash
        int subjectStart = (bracketEnd != -1) ? bracketEnd + 2 : (parts.size() >= 2 ? trimmedLine.indexOf(parts[1]) + parts[1].length() + 1 : -1);
        if (subjectStart > 0 && subjectStart < trimmedLine.length()) {
            branch.lastCommitSubject = trimmedLine.mid(subjectStart).trimmed();
        }

        branches.append(branch);
    }

    return branches;
}

void GitManager::checkoutBranch(const QString& branch)
{
    if (!m_isGitRepo || branch.isEmpty()) {
        return;
    }

    QStringList args = {"checkout", branch};
    emit operationStarted(tr("Switching to branch %1...").arg(branch));
    executeCommand(args, Operation::Checkout, branch);
}

void GitManager::createBranch(const QString& name, bool checkout)
{
    if (!m_isGitRepo || name.isEmpty()) {
        return;
    }

    QStringList args;
    if (checkout) {
        args = {"checkout", "-b", name};
    } else {
        args = {"branch", name};
    }
    emit operationStarted(tr("Creating branch %1...").arg(name));
    executeCommand(args, Operation::CreateBranch, name);
}

void GitManager::deleteBranch(const QString& name, bool force)
{
    if (!m_isGitRepo || name.isEmpty()) {
        return;
    }

    QStringList args = {"branch", force ? "-D" : "-d", name};
    emit operationStarted(tr("Deleting branch %1...").arg(name));
    executeCommand(args, Operation::DeleteBranch, name);
}

// ============================================================================
// History Operations
// ============================================================================

void GitManager::getLog(int maxCount, const QString& path)
{
    if (!m_isGitRepo) {
        return;
    }

    // Custom format for easy parsing
    // %H = full hash, %h = short hash, %an = author name, %ae = author email
    // %at = author timestamp, %s = subject, %b = body, %P = parent hashes
    QStringList args = {"log", QString("-n%1").arg(maxCount),
                        "--format=%H|%h|%an|%ae|%at|%s|%P"};
    if (!path.isEmpty()) {
        args << "--" << path;
    }
    executeCommand(args, Operation::Log);
}

QList<GitCommit> GitManager::parseLog(const QString& output)
{
    QList<GitCommit> commits;
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);

    for (const QString& line : lines) {
        QStringList parts = line.split('|');
        if (parts.size() >= 6) {
            GitCommit commit;
            commit.hash = parts[0];
            commit.shortHash = parts[1];
            commit.author = parts[2];
            commit.authorEmail = parts[3];
            commit.authorDate = QDateTime::fromSecsSinceEpoch(parts[4].toLongLong());
            commit.subject = parts[5];
            if (parts.size() >= 7) {
                commit.parentHashes = parts[6].split(' ', Qt::SkipEmptyParts);
            }
            commits.append(commit);
        }
    }

    return commits;
}

// ============================================================================
// Diff Operations
// ============================================================================

void GitManager::getDiff(const QString& path, bool staged)
{
    if (!m_isGitRepo) {
        return;
    }

    QStringList args = {"diff"};
    if (staged) {
        args << "--cached";
    }
    if (!path.isEmpty()) {
        args << "--" << path;
    }
    executeCommand(args, Operation::Diff);
}

} // namespace XXMLStudio
