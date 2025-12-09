#include "GitFetcher.h"

#include <QDir>
#include <QFileInfo>
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
