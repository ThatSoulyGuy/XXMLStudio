#ifndef GITFILEDECORATOR_H
#define GITFILEDECORATOR_H

#include <QIdentityProxyModel>
#include <QIcon>
#include <QHash>
#include "git/GitTypes.h"

namespace XXMLStudio {

class GitManager;

/**
 * Proxy model that adds Git status decorations to QFileSystemModel.
 * Used by ProjectExplorer to show file status indicators.
 *
 * Decorates file names with status colors:
 *   - Modified: Orange
 *   - Added: Green
 *   - Deleted: Red
 *   - Untracked: Gray
 */
class GitFileDecorator : public QIdentityProxyModel
{
    Q_OBJECT

public:
    explicit GitFileDecorator(QObject* parent = nullptr);
    ~GitFileDecorator();

    void setGitManager(GitManager* manager);
    void setRootPath(const QString& path);
    void setCompilationEntrypoint(const QString& relativePath);

    // QAbstractItemModel interface
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private slots:
    void onStatusRefreshed(const GitRepositoryStatus& status);
    void onRepositoryChanged(bool isGitRepo);

private:
    QColor statusColor(GitFileStatus status) const;
    QString getRelativePath(const QModelIndex& index) const;
    QIcon createEntrypointIcon(const QIcon& baseIcon) const;

    GitManager* m_gitManager = nullptr;
    QString m_rootPath;
    bool m_hasGitRepo = false;
    QString m_compilationEntrypoint;  // Relative path to entrypoint file

    // Cache: relative path -> status entry
    QHash<QString, GitStatusEntry> m_statusCache;
};

} // namespace XXMLStudio

#endif // GITFILEDECORATOR_H
