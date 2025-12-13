#ifndef GITHISTORYPANEL_H
#define GITHISTORYPANEL_H

#include <QWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLineEdit>
#include <QLabel>
#include <QSortFilterProxyModel>
#include "git/GitTypes.h"

namespace XXMLStudio {

class GitManager;

/**
 * Bottom panel showing commit history.
 * Columns: Hash | Author | Date | Message
 */
class GitHistoryPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GitHistoryPanel(QWidget* parent = nullptr);
    ~GitHistoryPanel();

    void setGitManager(GitManager* manager);
    void setFilePath(const QString& path); // Filter history to specific file
    void refresh();
    void clear();

signals:
    void commitSelected(const GitCommit& commit);
    void commitDoubleClicked(const GitCommit& commit);

private slots:
    void onLogReceived(const QList<GitCommit>& commits);
    void onRepositoryChanged(bool isGitRepo);
    void onItemDoubleClicked(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);
    void onRefreshClicked();

private:
    void setupUi();
    void addCommitToModel(const GitCommit& commit);

    GitManager* m_gitManager = nullptr;
    QString m_filePath;  // Empty = all history

    QVBoxLayout* m_layout = nullptr;
    QWidget* m_toolbarWidget = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QTableView* m_tableView = nullptr;
    QStandardItemModel* m_model = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    QLabel* m_noRepoLabel = nullptr;

    QList<GitCommit> m_commits;
};

} // namespace XXMLStudio

#endif // GITHISTORYPANEL_H
