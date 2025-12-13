#include "GitFetcher.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace XXMLStudio {

GitFetcher::GitFetcher(QObject* parent)
    : QObject(parent)
{
    m_gitExecutable = findGitExecutable();

    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GitFetcher::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &GitFetcher::onProcessError);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &GitFetcher::onReadyReadStdout);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &GitFetcher::onReadyReadStderr);

    // Set environment to prevent git from prompting for credentials
    // This prevents hangs when cloning from a non-interactive process
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_TERMINAL_PROMPT", "0");
    env.insert("GIT_ASKPASS", "");
    env.insert("SSH_ASKPASS", "");
    m_process->setProcessEnvironment(env);
}

GitFetcher::~GitFetcher()
{
    cancel();
}

QString GitFetcher::findGitExecutable()
{
    // Try to find git in PATH
    QString gitPath = QStandardPaths::findExecutable("git");
    if (!gitPath.isEmpty()) {
        return gitPath;
    }

    // Common Windows locations
    QStringList commonPaths = {
        "C:/Program Files/Git/bin/git.exe",
        "C:/Program Files (x86)/Git/bin/git.exe",
        "C:/Git/bin/git.exe"
    };

    for (const QString& path : commonPaths) {
        if (QFileInfo::exists(path)) {
            return path;
        }
    }

    return "git"; // Fall back to hoping it's in PATH
}

void GitFetcher::clone(const QString& url, const QString& targetPath, const QString& tag)
{
    if (isRunning()) {
        emit error("Another git operation is already in progress");
        return;
    }

    if (m_gitExecutable.isEmpty()) {
        emit error("Git executable not found. Please install Git.");
        return;
    }

    m_currentOperation = Operation::Clone;
    m_targetPath = targetPath;
    m_currentUrl = url;
    m_lastErrorOutput.clear();

    // Ensure parent directory exists
    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    QStringList args;
    args << "clone";
    args << "--depth" << "1"; // Shallow clone for faster downloads

    if (!tag.isEmpty()) {
        args << "--branch" << tag;
    }

    args << "--progress";
    args << url;
    args << targetPath;

    emit progress(QString("Cloning %1...").arg(url));
    startProcess(args);
}

void GitFetcher::checkout(const QString& repoPath, const QString& ref)
{
    if (isRunning()) {
        emit error("Another git operation is already in progress");
        return;
    }

    m_currentOperation = Operation::Checkout;
    m_targetPath = repoPath;

    QStringList args;
    args << "-C" << repoPath;
    args << "checkout" << ref;

    emit progress(QString("Checking out %1...").arg(ref));
    startProcess(args);
}

QString GitFetcher::getCurrentCommit(const QString& repoPath)
{
    QProcess process;
    process.setWorkingDirectory(repoPath);
    process.start(m_gitExecutable, {"rev-parse", "HEAD"});
    process.waitForFinished(5000);

    if (process.exitCode() == 0) {
        return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    }

    return QString();
}

bool GitFetcher::isGitRepository(const QString& path)
{
    return QDir(path + "/.git").exists();
}

void GitFetcher::cancel()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
    m_currentOperation = Operation::None;
}

bool GitFetcher::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void GitFetcher::startProcess(const QStringList& args)
{
    m_process->start(m_gitExecutable, args);
}

void GitFetcher::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    bool success = (status == QProcess::NormalExit && exitCode == 0);
    Operation op = m_currentOperation;
    m_currentOperation = Operation::None;

    if (success) {
        switch (op) {
        case Operation::Clone:
            emit progress("Clone completed successfully");
            break;
        case Operation::Checkout:
            emit progress("Checkout completed successfully");
            break;
        default:
            break;
        }
    } else {
        // Emit detailed error message
        QString errorMsg;
        switch (op) {
        case Operation::Clone:
            errorMsg = QString("Failed to clone repository: %1").arg(m_currentUrl);
            break;
        case Operation::Checkout:
            errorMsg = QString("Failed to checkout: %1").arg(m_targetPath);
            break;
        default:
            errorMsg = "Git operation failed";
            break;
        }

        // Include the actual git error output if available
        if (!m_lastErrorOutput.isEmpty()) {
            // Extract the most relevant error line (usually contains "fatal:" or "error:")
            QStringList lines = m_lastErrorOutput.split('\n', Qt::SkipEmptyParts);
            QString relevantError;
            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (trimmed.startsWith("fatal:") || trimmed.startsWith("error:")) {
                    relevantError = trimmed;
                    break;
                }
            }
            if (relevantError.isEmpty() && !lines.isEmpty()) {
                // Use the last non-progress line
                for (int i = lines.size() - 1; i >= 0; --i) {
                    QString trimmed = lines[i].trimmed();
                    // Skip progress lines (contain percentages)
                    if (!trimmed.contains('%') && !trimmed.isEmpty()) {
                        relevantError = trimmed;
                        break;
                    }
                }
            }
            if (!relevantError.isEmpty()) {
                errorMsg += "\n" + relevantError;
            }
        }

        emit error(errorMsg);
    }

    emit finished(success, m_targetPath);
}

void GitFetcher::onProcessError(QProcess::ProcessError processError)
{
    QString errorMessage;
    switch (processError) {
    case QProcess::FailedToStart:
        errorMessage = "Git failed to start. Please ensure Git is installed.";
        break;
    case QProcess::Crashed:
        errorMessage = "Git process crashed.";
        break;
    case QProcess::Timedout:
        errorMessage = "Git operation timed out.";
        break;
    case QProcess::WriteError:
        errorMessage = "Failed to write to Git process.";
        break;
    case QProcess::ReadError:
        errorMessage = "Failed to read from Git process.";
        break;
    default:
        errorMessage = "Unknown Git error occurred.";
        break;
    }

    emit error(errorMessage);
    m_currentOperation = Operation::None;
}

void GitFetcher::onReadyReadStdout()
{
    QString output = QString::fromUtf8(m_process->readAllStandardOutput());
    if (!output.trimmed().isEmpty()) {
        emit progress(output.trimmed());
    }
}

void GitFetcher::onReadyReadStderr()
{
    // Git outputs progress info to stderr
    QString output = QString::fromUtf8(m_process->readAllStandardError());

    // Accumulate for error reporting
    m_lastErrorOutput += output;

    if (!output.trimmed().isEmpty()) {
        // Filter out progress percentage lines for cleaner output
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty()) {
                emit progress(trimmed);
            }
        }
    }
}

} // namespace XXMLStudio
