#ifndef GITTYPES_H
#define GITTYPES_H

#include <QString>
#include <QDateTime>
#include <QList>
#include <QMetaType>

namespace XXMLStudio {

/**
 * Git file status flags.
 * Can represent status in index (staged) or working tree (unstaged).
 */
enum class GitFileStatus {
    Unmodified = 0,
    Modified,       // M - modified
    Added,          // A - added
    Deleted,        // D - deleted
    Renamed,        // R - renamed
    Copied,         // C - copied
    Untracked,      // ? - untracked
    Ignored,        // ! - ignored
    Conflicted,     // U - unmerged/conflicted
    TypeChanged     // T - type changed
};

/**
 * Represents a single file's Git status.
 */
struct GitStatusEntry {
    QString path;               // Relative path from repo root
    QString oldPath;            // For renames: original path
    GitFileStatus indexStatus = GitFileStatus::Unmodified;   // Status in staging area
    GitFileStatus workTreeStatus = GitFileStatus::Unmodified; // Status in working directory

    bool isStaged() const {
        return indexStatus != GitFileStatus::Unmodified &&
               indexStatus != GitFileStatus::Untracked;
    }

    bool isUnstaged() const {
        return workTreeStatus != GitFileStatus::Unmodified;
    }

    bool isUntracked() const {
        return indexStatus == GitFileStatus::Untracked ||
               workTreeStatus == GitFileStatus::Untracked;
    }

    bool isConflicted() const {
        return indexStatus == GitFileStatus::Conflicted ||
               workTreeStatus == GitFileStatus::Conflicted;
    }

    // Get display character for status
    static QChar statusChar(GitFileStatus status) {
        switch (status) {
            case GitFileStatus::Modified: return 'M';
            case GitFileStatus::Added: return 'A';
            case GitFileStatus::Deleted: return 'D';
            case GitFileStatus::Renamed: return 'R';
            case GitFileStatus::Copied: return 'C';
            case GitFileStatus::Untracked: return '?';
            case GitFileStatus::Ignored: return '!';
            case GitFileStatus::Conflicted: return 'U';
            case GitFileStatus::TypeChanged: return 'T';
            default: return ' ';
        }
    }

    // Get human-readable status string
    static QString statusString(GitFileStatus status) {
        switch (status) {
            case GitFileStatus::Modified: return QStringLiteral("Modified");
            case GitFileStatus::Added: return QStringLiteral("Added");
            case GitFileStatus::Deleted: return QStringLiteral("Deleted");
            case GitFileStatus::Renamed: return QStringLiteral("Renamed");
            case GitFileStatus::Copied: return QStringLiteral("Copied");
            case GitFileStatus::Untracked: return QStringLiteral("Untracked");
            case GitFileStatus::Ignored: return QStringLiteral("Ignored");
            case GitFileStatus::Conflicted: return QStringLiteral("Conflicted");
            case GitFileStatus::TypeChanged: return QStringLiteral("Type Changed");
            default: return QStringLiteral("Unmodified");
        }
    }
};

/**
 * Repository-wide status information.
 */
struct GitRepositoryStatus {
    QString branch;             // Current branch name
    QString upstream;           // Upstream tracking branch (e.g., "origin/main")
    int aheadCount = 0;         // Commits ahead of upstream
    int behindCount = 0;        // Commits behind upstream
    bool detachedHead = false;  // True if HEAD is detached
    bool mergeInProgress = false;
    bool rebaseInProgress = false;
    bool cherryPickInProgress = false;

    QList<GitStatusEntry> entries;

    // Convenience accessors
    QList<GitStatusEntry> stagedFiles() const {
        QList<GitStatusEntry> result;
        for (const GitStatusEntry& e : entries) {
            if (e.isStaged()) {
                result.append(e);
            }
        }
        return result;
    }

    QList<GitStatusEntry> unstagedFiles() const {
        QList<GitStatusEntry> result;
        for (const GitStatusEntry& e : entries) {
            if (e.isUnstaged() && !e.isUntracked()) {
                result.append(e);
            }
        }
        return result;
    }

    QList<GitStatusEntry> untrackedFiles() const {
        QList<GitStatusEntry> result;
        for (const GitStatusEntry& e : entries) {
            if (e.isUntracked()) {
                result.append(e);
            }
        }
        return result;
    }

    QList<GitStatusEntry> conflictedFiles() const {
        QList<GitStatusEntry> result;
        for (const GitStatusEntry& e : entries) {
            if (e.isConflicted()) {
                result.append(e);
            }
        }
        return result;
    }

    bool hasChanges() const {
        return !entries.isEmpty();
    }

    bool hasStagedChanges() const {
        for (const GitStatusEntry& e : entries) {
            if (e.isStaged()) return true;
        }
        return false;
    }
};

/**
 * Represents a Git commit.
 */
struct GitCommit {
    QString hash;               // Full SHA-1 hash (40 chars)
    QString shortHash;          // Short hash (7-8 chars)
    QString author;
    QString authorEmail;
    QDateTime authorDate;
    QString committer;
    QString committerEmail;
    QDateTime commitDate;
    QString subject;            // First line of commit message
    QString body;               // Rest of commit message
    QStringList parentHashes;   // Parent commit hashes

    bool isMergeCommit() const { return parentHashes.size() > 1; }
};

/**
 * Represents a Git branch.
 */
struct GitBranch {
    QString name;               // Branch name (e.g., "main", "feature/foo")
    QString fullRef;            // Full ref (e.g., "refs/heads/main")
    bool isRemote = false;      // True for remote tracking branches
    bool isCurrent = false;     // True if this is the current branch
    QString upstream;           // Upstream tracking branch
    QString lastCommitHash;
    QString lastCommitSubject;

    // Remote tracking info
    int aheadCount = 0;
    int behindCount = 0;
};

/**
 * Represents a Git remote.
 */
struct GitRemote {
    QString name;               // Remote name (e.g., "origin")
    QString fetchUrl;
    QString pushUrl;
};

/**
 * Result of a Git operation (for detailed error handling).
 */
struct GitOperationResult {
    bool success = false;
    QString output;
    QString errorOutput;
    int exitCode = -1;
};

} // namespace XXMLStudio

Q_DECLARE_METATYPE(XXMLStudio::GitFileStatus)
Q_DECLARE_METATYPE(XXMLStudio::GitStatusEntry)
Q_DECLARE_METATYPE(XXMLStudio::GitRepositoryStatus)
Q_DECLARE_METATYPE(XXMLStudio::GitCommit)
Q_DECLARE_METATYPE(XXMLStudio::GitBranch)

#endif // GITTYPES_H
