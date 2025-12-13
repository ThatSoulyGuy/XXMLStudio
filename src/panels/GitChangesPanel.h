#ifndef GITCHANGESPANEL_H
#define GITCHANGESPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QSplitter>
#include <QMenu>
#include "git/GitTypes.h"
#include "git/GitStatusModel.h"

namespace XXMLStudio {

class GitManager;

/**
 * Left sidebar panel showing Git changes with staging controls.
 * Layout:
 *   [Toolbar: Refresh | Stage All | Unstage All]
 *   [Remote Status: origin/main - 2 ahead, 1 behind] [Fetch] [Pull] [Push]
 *   [Splitter]
 *     [Changes Tree: Staged/Unstaged/Untracked sections]
 *     [Commit Area]
 *       [Commit message text edit]
 *       [Commit button]
 */
class GitChangesPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GitChangesPanel(QWidget* parent = nullptr);
    ~GitChangesPanel();

    void setGitManager(GitManager* manager);

signals:
    void fileDoubleClicked(const QString& path);
    void diffRequested(const QString& path, bool staged);

private slots:
    void onStatusRefreshed(const GitRepositoryStatus& status);
    void onRepositoryChanged(bool isGitRepo);
    void onStageClicked();
    void onUnstageClicked();
    void onStageAllClicked();
    void onUnstageAllClicked();
    void onDiscardClicked();
    void onRefreshClicked();
    void onCommitClicked();
    void onFetchClicked();
    void onPullClicked();
    void onPushClicked();
    void onItemDoubleClicked(const QModelIndex& index);
    void onSelectionChanged();
    void onCommitCompleted(bool success, const QString& hash, const QString& error);
    void onOperationError(const QString& error);
    void onContextMenuRequested(const QPoint& pos);
    void onPushNeedsUpstream(const QString& branch);
    void onRemotesReceived(const QStringList& remotes);
    void onRemoteAdded(bool success, const QString& name, const QString& error);
    void onInitClicked();
    void onInitCompleted(bool success, const QString& error);
    void onFetchCompleted(bool success, const QString& error);
    void onPullCompleted(bool success, const QString& error);
    void onPushCompleted(bool success, const QString& error);
    void onOperationStarted(const QString& operation);

private:
    void setupUi();
    void setupToolbar();
    void setupRemoteBar();
    void setupChangesTree();
    void setupCommitArea();
    void setupConnections();
    void updateRemoteStatus(const GitRepositoryStatus& status);
    void updateCommitButton();
    void updateInitButtonState();
    void showNoRepoMessage(bool show);
    void showStatusMessage(const QString& message, bool isError = false, int durationMs = 5000);
    QStringList selectedPaths();
    GitStatusModel::Section selectedSection();

    GitManager* m_gitManager = nullptr;

    // UI components
    QVBoxLayout* m_layout = nullptr;
    QWidget* m_contentWidget = nullptr;
    QWidget* m_noRepoWidget = nullptr;
    QLabel* m_noRepoLabel = nullptr;
    QPushButton* m_initRepoButton = nullptr;

    QToolBar* m_toolbar = nullptr;
    QWidget* m_remoteBar = nullptr;
    QLabel* m_branchLabel = nullptr;
    QLabel* m_remoteStatusLabel = nullptr;
    QToolButton* m_fetchButton = nullptr;
    QToolButton* m_pullButton = nullptr;
    QToolButton* m_pushButton = nullptr;

    QSplitter* m_splitter = nullptr;
    QTreeView* m_changesTree = nullptr;
    GitStatusModel* m_statusModel = nullptr;

    QWidget* m_commitArea = nullptr;
    QPlainTextEdit* m_commitMessage = nullptr;
    QPushButton* m_commitButton = nullptr;

    // Toolbar actions
    QAction* m_refreshAction = nullptr;
    QAction* m_stageAllAction = nullptr;
    QAction* m_unstageAllAction = nullptr;

    // Context menu actions
    QAction* m_stageAction = nullptr;
    QAction* m_unstageAction = nullptr;
    QAction* m_discardAction = nullptr;
    QAction* m_diffAction = nullptr;

    // State
    bool m_hasGitRepo = false;
    QString m_pendingBranch;      // Branch waiting for remote setup
    bool m_pendingIsPush = true;  // true for push, false for pull
    QStringList m_cachedRemotes;  // Cached list of remotes
};

} // namespace XXMLStudio

#endif // GITCHANGESPANEL_H
