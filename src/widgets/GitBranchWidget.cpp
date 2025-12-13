#include "GitBranchWidget.h"
#include "git/GitManager.h"
#include "core/IconUtils.h"

#include <QInputDialog>
#include <QMessageBox>

namespace XXMLStudio {

GitBranchWidget::GitBranchWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

GitBranchWidget::~GitBranchWidget()
{
}

void GitBranchWidget::setupUi()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(4);

    // Branch icon
    m_branchIcon = new QLabel(this);
    m_branchIcon->setPixmap(IconUtils::loadForDarkBackground(":/icons/Branch.svg").pixmap(16, 16));
    m_layout->addWidget(m_branchIcon);

    // Branch selector
    m_branchCombo = new QComboBox(this);
    m_branchCombo->setMinimumWidth(120);
    m_branchCombo->setMaximumWidth(200);
    m_branchCombo->setToolTip(tr("Current branch - click to switch"));
    m_branchCombo->setEnabled(false);
    m_layout->addWidget(m_branchCombo);

    // New branch button
    m_newBranchButton = new QToolButton(this);
    m_newBranchButton->setIcon(IconUtils::loadForDarkBackground(":/icons/Add.svg"));
    m_newBranchButton->setToolTip(tr("Create new branch"));
    m_newBranchButton->setEnabled(false);
    m_layout->addWidget(m_newBranchButton);

    // Connections
    connect(m_branchCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GitBranchWidget::onBranchSelected);
    connect(m_newBranchButton, &QToolButton::clicked,
            this, &GitBranchWidget::onNewBranchClicked);
}

void GitBranchWidget::setGitManager(GitManager* manager)
{
    if (m_gitManager) {
        disconnect(m_gitManager, nullptr, this, nullptr);
    }

    m_gitManager = manager;

    if (m_gitManager) {
        connect(m_gitManager, &GitManager::branchesReceived,
                this, &GitBranchWidget::onBranchesReceived);
        connect(m_gitManager, &GitManager::statusRefreshed,
                this, &GitBranchWidget::onStatusRefreshed);
        connect(m_gitManager, &GitManager::repositoryChanged,
                this, &GitBranchWidget::onRepositoryChanged);
        connect(m_gitManager, &GitManager::branchCheckoutCompleted,
                this, &GitBranchWidget::onBranchCheckoutCompleted);

        // Initial state
        bool hasRepo = m_gitManager->isGitRepository();
        m_branchCombo->setEnabled(hasRepo);
        m_newBranchButton->setEnabled(hasRepo);

        if (hasRepo) {
            m_gitManager->getBranches();
        }
    }
}

void GitBranchWidget::onRepositoryChanged(bool isGitRepo)
{
    m_branchCombo->setEnabled(isGitRepo);
    m_newBranchButton->setEnabled(isGitRepo);

    if (!isGitRepo) {
        m_ignoreSelectionChange = true;
        m_branchCombo->clear();
        m_ignoreSelectionChange = false;
        m_currentBranch.clear();
    } else if (m_gitManager) {
        m_gitManager->getBranches();
    }
}

void GitBranchWidget::onStatusRefreshed(const GitRepositoryStatus& status)
{
    if (status.branch != m_currentBranch) {
        m_currentBranch = status.branch;

        // Update combo box selection without triggering branch switch
        m_ignoreSelectionChange = true;
        int index = m_branchCombo->findText(status.branch);
        if (index >= 0) {
            m_branchCombo->setCurrentIndex(index);
        }
        m_ignoreSelectionChange = false;
    }
}

void GitBranchWidget::onBranchesReceived(const QList<GitBranch>& branches)
{
    QString currentBranch = m_gitManager ? m_gitManager->cachedStatus().branch : QString();
    updateBranchList(branches, currentBranch);
}

void GitBranchWidget::updateBranchList(const QList<GitBranch>& branches, const QString& current)
{
    m_ignoreSelectionChange = true;
    m_branchCombo->clear();

    // Add local branches first
    for (const GitBranch& branch : branches) {
        if (!branch.isRemote) {
            m_branchCombo->addItem(branch.name, branch.name);
        }
    }

    // Add separator if we have remote branches
    bool hasRemote = false;
    for (const GitBranch& branch : branches) {
        if (branch.isRemote) {
            hasRemote = true;
            break;
        }
    }

    if (hasRemote && m_branchCombo->count() > 0) {
        m_branchCombo->insertSeparator(m_branchCombo->count());
    }

    // Add remote branches
    for (const GitBranch& branch : branches) {
        if (branch.isRemote) {
            m_branchCombo->addItem(branch.name, branch.name);
        }
    }

    // Select current branch
    int index = m_branchCombo->findText(current);
    if (index >= 0) {
        m_branchCombo->setCurrentIndex(index);
    }

    m_currentBranch = current;
    m_ignoreSelectionChange = false;
}

void GitBranchWidget::onBranchSelected(int index)
{
    if (m_ignoreSelectionChange || index < 0) {
        return;
    }

    QString branch = m_branchCombo->itemData(index).toString();
    if (branch.isEmpty() || branch == m_currentBranch) {
        return;
    }

    // Check if it's a remote branch (e.g., "origin/feature")
    if (branch.contains('/')) {
        // For remote branches, create a local tracking branch
        QString localName = branch.section('/', -1);

        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            tr("Checkout Remote Branch"),
            tr("Create local branch '%1' tracking '%2'?").arg(localName, branch),
            QMessageBox::Yes | QMessageBox::No);

        if (result == QMessageBox::Yes && m_gitManager) {
            // Would need to implement: git checkout -b localName --track remoteBranch
            // For now, just checkout the remote
            m_gitManager->checkoutBranch(branch);
        } else {
            // Revert combo selection
            m_ignoreSelectionChange = true;
            int currentIndex = m_branchCombo->findText(m_currentBranch);
            if (currentIndex >= 0) {
                m_branchCombo->setCurrentIndex(currentIndex);
            }
            m_ignoreSelectionChange = false;
        }
    } else {
        if (m_gitManager) {
            m_gitManager->checkoutBranch(branch);
        }
        emit branchSwitchRequested(branch);
    }
}

void GitBranchWidget::onNewBranchClicked()
{
    bool ok;
    QString name = QInputDialog::getText(
        this,
        tr("New Branch"),
        tr("Enter branch name:"),
        QLineEdit::Normal,
        QString(),
        &ok);

    if (ok && !name.isEmpty()) {
        // Validate branch name (basic validation)
        if (name.contains(' ') || name.contains("..") || name.startsWith('-')) {
            QMessageBox::warning(this, tr("Invalid Branch Name"),
                tr("Branch name cannot contain spaces, start with '-', or contain '..'"));
            return;
        }

        if (m_gitManager) {
            m_gitManager->createBranch(name, true);  // Create and checkout
        }
        emit newBranchRequested();
    }
}

void GitBranchWidget::onBranchCheckoutCompleted(bool success, const QString& branch, const QString& error)
{
    if (success) {
        m_currentBranch = branch;
        // Refresh branches to update the list
        if (m_gitManager) {
            m_gitManager->getBranches();
        }
    } else {
        // Revert combo selection on failure
        m_ignoreSelectionChange = true;
        int index = m_branchCombo->findText(m_currentBranch);
        if (index >= 0) {
            m_branchCombo->setCurrentIndex(index);
        }
        m_ignoreSelectionChange = false;

        QMessageBox::warning(this, tr("Branch Switch Failed"), error);
    }
}

} // namespace XXMLStudio
