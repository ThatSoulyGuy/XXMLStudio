#include "GitFileDecorator.h"
#include "git/GitManager.h"

#include <QFileSystemModel>
#include <QFileInfo>
#include <QDir>
#include <QPainter>
#include <QPixmap>

namespace XXMLStudio {

GitFileDecorator::GitFileDecorator(QObject* parent)
    : QIdentityProxyModel(parent)
{
}

GitFileDecorator::~GitFileDecorator()
{
}

void GitFileDecorator::setGitManager(GitManager* manager)
{
    if (m_gitManager) {
        disconnect(m_gitManager, nullptr, this, nullptr);
    }

    m_gitManager = manager;

    if (m_gitManager) {
        connect(m_gitManager, &GitManager::statusRefreshed,
                this, &GitFileDecorator::onStatusRefreshed);
        connect(m_gitManager, &GitManager::repositoryChanged,
                this, &GitFileDecorator::onRepositoryChanged);

        m_hasGitRepo = m_gitManager->isGitRepository();
    }
}

void GitFileDecorator::setRootPath(const QString& path)
{
    m_rootPath = path;
    m_statusCache.clear();
}

void GitFileDecorator::setCompilationEntrypoint(const QString& relativePath)
{
    if (m_compilationEntrypoint != relativePath) {
        m_compilationEntrypoint = relativePath;
        // Normalize to forward slashes
        m_compilationEntrypoint = m_compilationEntrypoint.replace('\\', '/');
        // Emit data changed for all items to refresh icons
        if (sourceModel()) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    }
}

void GitFileDecorator::onRepositoryChanged(bool isGitRepo)
{
    m_hasGitRepo = isGitRepo;
    if (!isGitRepo) {
        m_statusCache.clear();
        // Emit data changed for all items
        if (sourceModel()) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
        }
    }
}

void GitFileDecorator::onStatusRefreshed(const GitRepositoryStatus& status)
{
    m_statusCache.clear();

    for (const GitStatusEntry& entry : status.entries) {
        m_statusCache[entry.path] = entry;
    }

    // Emit data changed for all items
    if (sourceModel()) {
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
}

QString GitFileDecorator::getRelativePath(const QModelIndex& index) const
{
    if (!sourceModel() || m_rootPath.isEmpty()) {
        return QString();
    }

    // Get the file path from the source model (QFileSystemModel)
    QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
    if (!fsModel) {
        return QString();
    }

    QString filePath = fsModel->filePath(mapToSource(index));
    if (filePath.isEmpty()) {
        return QString();
    }

    // Make path relative to root
    QDir rootDir(m_rootPath);
    QString relativePath = rootDir.relativeFilePath(filePath);

    // Normalize to forward slashes (Git uses forward slashes)
    relativePath = relativePath.replace('\\', '/');

    return relativePath;
}

QVariant GitFileDecorator::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    // Only decorate the first column (file name)
    if (index.column() != 0) {
        return QIdentityProxyModel::data(index, role);
    }

    // Add icon overlay for compilation entrypoint
    if (role == Qt::DecorationRole && !m_compilationEntrypoint.isEmpty()) {
        QString relativePath = getRelativePath(index);
        if (!relativePath.isEmpty() && relativePath == m_compilationEntrypoint) {
            QIcon baseIcon = QIdentityProxyModel::data(index, role).value<QIcon>();
            return createEntrypointIcon(baseIcon);
        }
    }

    // Add foreground color based on Git status
    if (role == Qt::ForegroundRole && m_hasGitRepo) {
        QString relativePath = getRelativePath(index);

        if (!relativePath.isEmpty() && m_statusCache.contains(relativePath)) {
            const GitStatusEntry& entry = m_statusCache[relativePath];

            // Use work tree status if unstaged, otherwise index status
            GitFileStatus status = GitFileStatus::Unmodified;
            if (entry.isUntracked()) {
                status = GitFileStatus::Untracked;
            } else if (entry.isUnstaged()) {
                status = entry.workTreeStatus;
            } else if (entry.isStaged()) {
                status = entry.indexStatus;
            }

            if (status != GitFileStatus::Unmodified) {
                return statusColor(status);
            }
        }

        // Check if any parent directory has this file as part of its path
        // (e.g., if "src/foo.cpp" is modified, we might want to mark "src" folder)
        for (auto it = m_statusCache.begin(); it != m_statusCache.end(); ++it) {
            if (it.key().startsWith(relativePath + "/")) {
                // This is a directory containing modified files
                // Use a subtle indicator (dimmed orange)
                return QColor("#8b7355");
            }
        }
    }

    return QIdentityProxyModel::data(index, role);
}

QColor GitFileDecorator::statusColor(GitFileStatus status) const
{
    switch (status) {
    case GitFileStatus::Modified:
        return QColor("#e2c08d");  // Orange/yellow
    case GitFileStatus::Added:
        return QColor("#73c991");  // Green
    case GitFileStatus::Deleted:
        return QColor("#f14c4c");  // Red
    case GitFileStatus::Renamed:
        return QColor("#4fc1ff");  // Cyan
    case GitFileStatus::Untracked:
        return QColor("#888888");  // Gray
    case GitFileStatus::Conflicted:
        return QColor("#f14c4c");  // Red
    default:
        return QColor();  // Use default
    }
}

QIcon GitFileDecorator::createEntrypointIcon(const QIcon& baseIcon) const
{
    // Create icon with multiple sizes for proper scaling
    QIcon resultIcon;

    // Generate for common icon sizes
    for (int iconSize : {16, 22, 24, 32}) {
        QPixmap basePixmap = baseIcon.pixmap(iconSize, iconSize);

        if (basePixmap.isNull()) {
            continue;
        }

        // Create a new pixmap to draw on
        QPixmap decoratedPixmap(basePixmap.size());
        decoratedPixmap.fill(Qt::transparent);

        QPainter painter(&decoratedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw the base icon
        painter.drawPixmap(0, 0, basePixmap);

        // Draw a green play triangle in the top-left corner
        // Scale badge size proportionally
        const int badgeSize = qMax(7, iconSize / 3);
        const int badgeX = 0;
        const int badgeY = 0;

        // Draw triangle background (play icon)
        painter.setBrush(QColor("#4ec9b0"));  // Teal/green color
        painter.setPen(QPen(QColor("#1e1e1e"), 1));

        QPolygonF triangle;
        triangle << QPointF(badgeX + 1, badgeY + 1)
                 << QPointF(badgeX + badgeSize, badgeY + badgeSize / 2.0 + 0.5)
                 << QPointF(badgeX + 1, badgeY + badgeSize);
        painter.drawPolygon(triangle);

        painter.end();

        resultIcon.addPixmap(decoratedPixmap);
    }

    return resultIcon.isNull() ? baseIcon : resultIcon;
}

} // namespace XXMLStudio
