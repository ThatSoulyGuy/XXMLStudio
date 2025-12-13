#ifndef GITBRANCHWIDGET_H
#define GITBRANCHWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QToolButton>
#include <QHBoxLayout>
#include <QLabel>
#include "git/GitTypes.h"

namespace XXMLStudio {

class GitManager;

/**
 * Toolbar widget showing current branch with dropdown to switch.
 * Shows: [Branch Icon] [branch-name v] [+] (new branch)
 */
class GitBranchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GitBranchWidget(QWidget* parent = nullptr);
    ~GitBranchWidget();

    void setGitManager(GitManager* manager);

signals:
    void branchSwitchRequested(const QString& branch);
    void newBranchRequested();

private slots:
    void onBranchesReceived(const QList<GitBranch>& branches);
    void onStatusRefreshed(const GitRepositoryStatus& status);
    void onRepositoryChanged(bool isGitRepo);
    void onBranchSelected(int index);
    void onNewBranchClicked();
    void onBranchCheckoutCompleted(bool success, const QString& branch, const QString& error);

private:
    void setupUi();
    void updateBranchList(const QList<GitBranch>& branches, const QString& current);

    GitManager* m_gitManager = nullptr;

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_branchIcon = nullptr;
    QComboBox* m_branchCombo = nullptr;
    QToolButton* m_newBranchButton = nullptr;

    QString m_currentBranch;
    bool m_ignoreSelectionChange = false;
};

} // namespace XXMLStudio

#endif // GITBRANCHWIDGET_H
