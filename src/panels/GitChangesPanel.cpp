#include "GitChangesPanel.h"
#include "git/GitManager.h"
#include "git/GitStatusModel.h"
#include "core/IconUtils.h"
#include "dialogs/SetUpstreamDialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>

namespace XXMLStudio {

GitChangesPanel::GitChangesPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

GitChangesPanel::~GitChangesPanel()
{
}

void GitChangesPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // No repo widget (shown when not in a Git repo)
    m_noRepoWidget = new QWidget(this);
    QVBoxLayout* noRepoLayout = new QVBoxLayout(m_noRepoWidget);
    noRepoLayout->setContentsMargins(20, 20, 20, 20);
    noRepoLayout->setSpacing(16);

    noRepoLayout->addStretch();

    m_noRepoLabel = new QLabel(tr("No Git repository detected.\n\nOpen a project that is a Git repository,\nor initialize a new repository."), this);
    m_noRepoLabel->setAlignment(Qt::AlignCenter);
    m_noRepoLabel->setWordWrap(true);
    m_noRepoLabel->setStyleSheet("color: #888;");
    noRepoLayout->addWidget(m_noRepoLabel);

    m_initRepoButton = new QPushButton(tr("Initialize Repository"), this);
    m_initRepoButton->setIcon(IconUtils::loadForDarkBackground(":/icons/Add.svg"));
    m_initRepoButton->setToolTip(tr("Create a new Git repository in the current project directory"));
    noRepoLayout->addWidget(m_initRepoButton, 0, Qt::AlignCenter);

    noRepoLayout->addStretch();

    m_layout->addWidget(m_noRepoWidget);

    // Main content widget (hidden when no repo)
    m_contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    setupToolbar();
    contentLayout->addWidget(m_toolbar);

    setupRemoteBar();
    contentLayout->addWidget(m_remoteBar);

    // Splitter for tree and commit area
    m_splitter = new QSplitter(Qt::Vertical, this);

    setupChangesTree();
    m_splitter->addWidget(m_changesTree);

    setupCommitArea();
    m_splitter->addWidget(m_commitArea);

    // Set splitter proportions
    m_splitter->setStretchFactor(0, 3);  // Tree gets more space
    m_splitter->setStretchFactor(1, 1);  // Commit area gets less

    contentLayout->addWidget(m_splitter, 1);

    m_layout->addWidget(m_contentWidget);

    // Start with no repo message visible
    showNoRepoMessage(true);

    setupConnections();
}

void GitChangesPanel::setupToolbar()
{
    m_toolbar = new QToolBar(this);
    m_toolbar->setIconSize(QSize(16, 16));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    m_refreshAction = m_toolbar->addAction(tr("Refresh"));
    m_refreshAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Refresh.svg"));
    m_refreshAction->setToolTip(tr("Refresh status (F5)"));
    m_refreshAction->setShortcut(QKeySequence::Refresh);

    m_toolbar->addSeparator();

    m_stageAllAction = m_toolbar->addAction(tr("Stage All"));
    m_stageAllAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Add.svg"));
    m_stageAllAction->setToolTip(tr("Stage all changes"));

    m_unstageAllAction = m_toolbar->addAction(tr("Unstage All"));
    m_unstageAllAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Remove.svg"));
    m_unstageAllAction->setToolTip(tr("Unstage all changes"));
}

void GitChangesPanel::setupRemoteBar()
{
    m_remoteBar = new QWidget(this);
    QHBoxLayout* remoteLayout = new QHBoxLayout(m_remoteBar);
    remoteLayout->setContentsMargins(8, 4, 8, 4);
    remoteLayout->setSpacing(8);

    m_branchLabel = new QLabel(this);
    m_branchLabel->setStyleSheet("font-weight: bold;");
    remoteLayout->addWidget(m_branchLabel);

    m_remoteStatusLabel = new QLabel(this);
    m_remoteStatusLabel->setStyleSheet("color: #888;");
    remoteLayout->addWidget(m_remoteStatusLabel);

    remoteLayout->addStretch();

    m_fetchButton = new QToolButton(this);
    m_fetchButton->setText(tr("Fetch"));
    m_fetchButton->setIcon(IconUtils::loadForDarkBackground(":/icons/CloudDownload.svg"));
    m_fetchButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_fetchButton->setToolTip(tr("Fetch from remote"));
    remoteLayout->addWidget(m_fetchButton);

    m_pullButton = new QToolButton(this);
    m_pullButton->setText(tr("Pull"));
    m_pullButton->setIcon(IconUtils::loadForDarkBackground(":/icons/ArrowDownEnd.svg"));
    m_pullButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pullButton->setToolTip(tr("Pull changes from remote"));
    remoteLayout->addWidget(m_pullButton);

    m_pushButton = new QToolButton(this);
    m_pushButton->setText(tr("Push"));
    m_pushButton->setIcon(IconUtils::loadForDarkBackground(":/icons/ArrowUpEnd.svg"));
    m_pushButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pushButton->setToolTip(tr("Push changes to remote"));
    remoteLayout->addWidget(m_pushButton);
}

void GitChangesPanel::setupChangesTree()
{
    m_changesTree = new QTreeView(this);
    m_statusModel = new GitStatusModel(this);

    m_changesTree->setModel(m_statusModel);
    m_changesTree->setHeaderHidden(true);
    m_changesTree->setRootIsDecorated(true);
    m_changesTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_changesTree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_changesTree->setAnimated(true);

    // Expand all sections by default
    m_changesTree->expandAll();

    // Context menu actions
    m_stageAction = new QAction(tr("Stage"), this);
    m_stageAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Add.svg"));

    m_unstageAction = new QAction(tr("Unstage"), this);
    m_unstageAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Remove.svg"));

    m_discardAction = new QAction(tr("Discard Changes"), this);
    m_discardAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Undo.svg"));

    m_diffAction = new QAction(tr("View Diff"), this);
}

void GitChangesPanel::setupCommitArea()
{
    m_commitArea = new QWidget(this);
    QVBoxLayout* commitLayout = new QVBoxLayout(m_commitArea);
    commitLayout->setContentsMargins(8, 8, 8, 8);
    commitLayout->setSpacing(8);

    m_commitMessage = new QPlainTextEdit(this);
    m_commitMessage->setPlaceholderText(tr("Commit message..."));
    m_commitMessage->setMaximumHeight(100);
    commitLayout->addWidget(m_commitMessage);

    m_commitButton = new QPushButton(tr("Commit"), this);
    m_commitButton->setIcon(IconUtils::loadForDarkBackground(":/icons/Checkmark.svg"));
    m_commitButton->setEnabled(false);
    commitLayout->addWidget(m_commitButton);
}

void GitChangesPanel::setupConnections()
{
    // Init button
    connect(m_initRepoButton, &QPushButton::clicked, this, &GitChangesPanel::onInitClicked);

    // Toolbar actions
    connect(m_refreshAction, &QAction::triggered, this, &GitChangesPanel::onRefreshClicked);
    connect(m_stageAllAction, &QAction::triggered, this, &GitChangesPanel::onStageAllClicked);
    connect(m_unstageAllAction, &QAction::triggered, this, &GitChangesPanel::onUnstageAllClicked);

    // Remote buttons
    connect(m_fetchButton, &QToolButton::clicked, this, &GitChangesPanel::onFetchClicked);
    connect(m_pullButton, &QToolButton::clicked, this, &GitChangesPanel::onPullClicked);
    connect(m_pushButton, &QToolButton::clicked, this, &GitChangesPanel::onPushClicked);

    // Tree interactions
    connect(m_changesTree, &QTreeView::doubleClicked, this, &GitChangesPanel::onItemDoubleClicked);
    connect(m_changesTree->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &GitChangesPanel::onSelectionChanged);
    connect(m_changesTree, &QTreeView::customContextMenuRequested,
            this, &GitChangesPanel::onContextMenuRequested);

    // Context menu actions
    connect(m_stageAction, &QAction::triggered, this, &GitChangesPanel::onStageClicked);
    connect(m_unstageAction, &QAction::triggered, this, &GitChangesPanel::onUnstageClicked);
    connect(m_discardAction, &QAction::triggered, this, &GitChangesPanel::onDiscardClicked);

    // Commit
    connect(m_commitButton, &QPushButton::clicked, this, &GitChangesPanel::onCommitClicked);
    connect(m_commitMessage, &QPlainTextEdit::textChanged, this, &GitChangesPanel::updateCommitButton);
}

void GitChangesPanel::setGitManager(GitManager* manager)
{
    qDebug() << "[GitChangesPanel] setGitManager called";

    if (m_gitManager) {
        disconnect(m_gitManager, nullptr, this, nullptr);
    }

    m_gitManager = manager;

    if (m_gitManager) {
        qDebug() << "[GitChangesPanel] Connecting to GitManager signals";
        connect(m_gitManager, &GitManager::statusRefreshed,
                this, &GitChangesPanel::onStatusRefreshed);
        connect(m_gitManager, &GitManager::repositoryChanged,
                this, &GitChangesPanel::onRepositoryChanged);
        connect(m_gitManager, &GitManager::commitCompleted,
                this, &GitChangesPanel::onCommitCompleted);
        connect(m_gitManager, &GitManager::operationError,
                this, &GitChangesPanel::onOperationError);
        connect(m_gitManager, &GitManager::pushNeedsUpstream,
                this, &GitChangesPanel::onPushNeedsUpstream);
        connect(m_gitManager, &GitManager::remotesReceived,
                this, &GitChangesPanel::onRemotesReceived);
        connect(m_gitManager, &GitManager::remoteAdded,
                this, &GitChangesPanel::onRemoteAdded);
        connect(m_gitManager, &GitManager::initCompleted,
                this, &GitChangesPanel::onInitCompleted);
        connect(m_gitManager, &GitManager::fetchCompleted,
                this, &GitChangesPanel::onFetchCompleted);
        connect(m_gitManager, &GitManager::pullCompleted,
                this, &GitChangesPanel::onPullCompleted);
        connect(m_gitManager, &GitManager::pushCompleted,
                this, &GitChangesPanel::onPushCompleted);
        connect(m_gitManager, &GitManager::operationStarted,
                this, &GitChangesPanel::onOperationStarted);

        // Initial state
        m_hasGitRepo = m_gitManager->isGitRepository();
        qDebug() << "[GitChangesPanel] Initial hasGitRepo:" << m_hasGitRepo;
        showNoRepoMessage(!m_hasGitRepo);
    }
}

void GitChangesPanel::showNoRepoMessage(bool show)
{
    m_noRepoWidget->setVisible(show);
    m_contentWidget->setVisible(!show);

    if (show) {
        updateInitButtonState();
    }
}

void GitChangesPanel::updateInitButtonState()
{
    if (!m_gitManager) {
        m_initRepoButton->setEnabled(false);
        return;
    }

    bool hasPath = !m_gitManager->repositoryPath().isEmpty();
    bool isAlreadyRepo = m_gitManager->isGitRepository();

    qDebug() << "[GitChangesPanel] updateInitButtonState: hasPath=" << hasPath
             << "isAlreadyRepo=" << isAlreadyRepo;

    // Enable button only if we have a path and it's not already a git repo
    m_initRepoButton->setEnabled(hasPath && !isAlreadyRepo);
}

void GitChangesPanel::onRepositoryChanged(bool isGitRepo)
{
    qDebug() << "[GitChangesPanel] onRepositoryChanged:" << isGitRepo;
    m_hasGitRepo = isGitRepo;
    showNoRepoMessage(!isGitRepo);

    if (!isGitRepo) {
        m_statusModel->clear();
        m_branchLabel->clear();
        m_remoteStatusLabel->clear();
    }

    // Update init button state based on whether we have a project path
    updateInitButtonState();
}

void GitChangesPanel::onStatusRefreshed(const GitRepositoryStatus& status)
{
    qDebug() << "[GitChangesPanel] onStatusRefreshed - branch:" << status.branch
             << "entries:" << status.entries.size();
    m_statusModel->setStatus(status);

    // Expand all sections after refresh
    m_changesTree->expandAll();

    qDebug() << "[GitChangesPanel] Model row count at root:" << m_statusModel->rowCount()
             << "staged:" << m_statusModel->stagedEntries().size()
             << "unstaged:" << m_statusModel->unstagedEntries().size()
             << "untracked:" << m_statusModel->untrackedEntries().size();

    updateRemoteStatus(status);
    updateCommitButton();
}

void GitChangesPanel::updateRemoteStatus(const GitRepositoryStatus& status)
{
    // Branch label
    QString branchText = status.detachedHead ? tr("HEAD detached") : status.branch;
    m_branchLabel->setText(branchText);

    // Remote status
    QStringList statusParts;
    if (!status.upstream.isEmpty()) {
        if (status.aheadCount > 0) {
            statusParts << QString("%1↑").arg(status.aheadCount);
        }
        if (status.behindCount > 0) {
            statusParts << QString("%1↓").arg(status.behindCount);
        }
        if (statusParts.isEmpty()) {
            statusParts << tr("up to date");
        }
    } else {
        statusParts << tr("no upstream");
    }

    m_remoteStatusLabel->setText(statusParts.join("  "));

    // Enable/disable push button based on ahead count
    m_pushButton->setEnabled(status.aheadCount > 0 || status.upstream.isEmpty());

    // Enable/disable pull button based on behind count
    m_pullButton->setEnabled(status.behindCount > 0);
}

void GitChangesPanel::updateCommitButton()
{
    bool hasMessage = !m_commitMessage->toPlainText().trimmed().isEmpty();
    bool hasStaged = !m_statusModel->stagedEntries().isEmpty();
    m_commitButton->setEnabled(hasMessage && hasStaged);

    // Update button text to show branch
    if (m_gitManager && m_gitManager->isGitRepository()) {
        QString branch = m_gitManager->cachedStatus().branch;
        if (!branch.isEmpty()) {
            m_commitButton->setText(tr("Commit to %1").arg(branch));
        } else {
            m_commitButton->setText(tr("Commit"));
        }
    }
}

QStringList GitChangesPanel::selectedPaths()
{
    return m_statusModel->pathsForIndices(m_changesTree->selectionModel()->selectedIndexes());
}

GitStatusModel::Section GitChangesPanel::selectedSection()
{
    QModelIndexList selected = m_changesTree->selectionModel()->selectedIndexes();
    if (!selected.isEmpty()) {
        return m_statusModel->sectionAt(selected.first());
    }
    return GitStatusModel::Section::Staged;
}

void GitChangesPanel::onSelectionChanged()
{
    // Could update context-sensitive UI here
}

void GitChangesPanel::onStageClicked()
{
    QStringList paths = selectedPaths();
    if (!paths.isEmpty() && m_gitManager) {
        m_gitManager->stage(paths);
    }
}

void GitChangesPanel::onUnstageClicked()
{
    QStringList paths = selectedPaths();
    if (!paths.isEmpty() && m_gitManager) {
        m_gitManager->unstage(paths);
    }
}

void GitChangesPanel::onStageAllClicked()
{
    if (m_gitManager) {
        m_gitManager->stageAll();
    }
}

void GitChangesPanel::onUnstageAllClicked()
{
    if (m_gitManager) {
        m_gitManager->unstageAll();
    }
}

void GitChangesPanel::onDiscardClicked()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) {
        return;
    }

    QMessageBox::StandardButton result = QMessageBox::warning(
        this,
        tr("Discard Changes"),
        tr("Are you sure you want to discard changes to %n file(s)?\n\nThis action cannot be undone.", "", paths.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (result == QMessageBox::Yes && m_gitManager) {
        m_gitManager->discardChanges(paths);
    }
}

void GitChangesPanel::onRefreshClicked()
{
    if (m_gitManager) {
        m_gitManager->refreshStatus();
    }
}

void GitChangesPanel::onCommitClicked()
{
    QString message = m_commitMessage->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    if (m_gitManager) {
        m_gitManager->commit(message);
    }
}

void GitChangesPanel::onFetchClicked()
{
    if (m_gitManager) {
        m_gitManager->fetch();
    }
}

void GitChangesPanel::onPullClicked()
{
    qDebug() << "[GitChangesPanel] onPullClicked()";
    if (!m_gitManager) {
        return;
    }

    // Check if we need to set up a remote first
    m_pendingBranch = m_gitManager->cachedStatus().branch;
    if (m_pendingBranch.isEmpty()) {
        m_pendingBranch = "main";
    }
    m_pendingIsPush = false;  // This is a pull

    qDebug() << "[GitChangesPanel] Checking remotes before pull, branch:" << m_pendingBranch;
    m_gitManager->getRemotes();
}

void GitChangesPanel::onPushClicked()
{
    qDebug() << "[GitChangesPanel] onPushClicked()";
    if (!m_gitManager) {
        return;
    }

    // Check if we need to set up a remote first
    m_pendingBranch = m_gitManager->cachedStatus().branch;
    if (m_pendingBranch.isEmpty()) {
        m_pendingBranch = "main";
    }
    m_pendingIsPush = true;  // This is a push

    qDebug() << "[GitChangesPanel] Checking remotes before push, branch:" << m_pendingBranch;
    m_gitManager->getRemotes();
}

void GitChangesPanel::onItemDoubleClicked(const QModelIndex& index)
{
    if (m_statusModel->isHeader(index)) {
        return;  // Don't handle double-click on headers
    }

    GitStatusEntry entry = m_statusModel->entryAt(index);
    if (!entry.path.isEmpty()) {
        emit fileDoubleClicked(entry.path);
    }
}

void GitChangesPanel::onCommitCompleted(bool success, const QString& hash, const QString& error)
{
    if (success) {
        m_commitMessage->clear();
        // Could show a brief success message
    } else {
        QMessageBox::warning(this, tr("Commit Failed"), error);
    }
}

void GitChangesPanel::onOperationError(const QString& error)
{
    qDebug() << "[GitChangesPanel] onOperationError:" << error;
    showStatusMessage(tr("Error: %1").arg(error), true);
}

void GitChangesPanel::showStatusMessage(const QString& message, bool isError, int durationMs)
{
    if (!m_branchLabel) {
        return;
    }

    if (isError) {
        m_branchLabel->setStyleSheet("font-weight: bold; color: #f14c4c;");
    } else {
        m_branchLabel->setStyleSheet("font-weight: bold; color: #4ec9b0;");
    }
    m_branchLabel->setText(message);

    // Reset after duration
    QTimer::singleShot(durationMs, this, [this]() {
        m_branchLabel->setStyleSheet("font-weight: bold;");
        if (m_gitManager) {
            m_branchLabel->setText(m_gitManager->cachedStatus().branch);
        }
    });
}

void GitChangesPanel::onContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = m_changesTree->indexAt(pos);
    if (!index.isValid() || m_statusModel->isHeader(index)) {
        return;
    }

    GitStatusModel::Section section = m_statusModel->sectionAt(index);
    QMenu menu(this);

    switch (section) {
    case GitStatusModel::Section::Staged:
        menu.addAction(m_unstageAction);
        break;
    case GitStatusModel::Section::Unstaged:
        menu.addAction(m_stageAction);
        menu.addAction(m_discardAction);
        break;
    case GitStatusModel::Section::Untracked:
        menu.addAction(m_stageAction);
        menu.addAction(m_discardAction);
        break;
    default:
        break;
    }

    menu.addSeparator();
    menu.addAction(m_diffAction);

    menu.exec(m_changesTree->viewport()->mapToGlobal(pos));
}

void GitChangesPanel::onPushNeedsUpstream(const QString& branch)
{
    qDebug() << "[GitChangesPanel] onPushNeedsUpstream for branch:" << branch;

    // Store the branch for after we get the remotes list
    m_pendingBranch = branch;
    m_pendingIsPush = true;

    // Request current remotes list
    if (m_gitManager) {
        m_gitManager->getRemotes();
    }
}

void GitChangesPanel::onRemotesReceived(const QStringList& remotes)
{
    qDebug() << "[GitChangesPanel] onRemotesReceived: remotes=" << remotes
             << "pendingBranch=" << m_pendingBranch
             << "isPush=" << m_pendingIsPush;

    m_cachedRemotes = remotes;

    // If we have a pending operation
    if (!m_pendingBranch.isEmpty()) {
        QString branch = m_pendingBranch;
        bool isPush = m_pendingIsPush;

        // If no remotes configured, must show dialog
        if (remotes.isEmpty()) {
            qDebug() << "[GitChangesPanel] No remotes configured, showing SetUpstreamDialog";
            m_pendingBranch.clear();

            SetUpstreamDialog dialog(branch, remotes, this);
            if (dialog.exec() == QDialog::Accepted) {
                QString remoteName = dialog.remoteName();
                QString remoteUrl = dialog.remoteUrl();

                qDebug() << "[GitChangesPanel] Dialog accepted - remoteName=" << remoteName << "remoteUrl=" << remoteUrl;

                if (remoteUrl.isEmpty()) {
                    qDebug() << "[GitChangesPanel] ERROR: Remote URL is empty!";
                    QMessageBox::warning(this, tr("Invalid Remote"), tr("Remote URL cannot be empty."));
                    return;
                }

                // Store branch for after remote is added
                m_pendingBranch = branch;
                m_pendingIsPush = isPush;
                qDebug() << "[GitChangesPanel] Calling addRemote(" << remoteName << "," << remoteUrl << ")";
                m_gitManager->addRemote(remoteName, remoteUrl);
            } else {
                qDebug() << "[GitChangesPanel] Dialog cancelled by user";
            }
        } else {
            // Has remotes - proceed with operation
            qDebug() << "[GitChangesPanel] Remotes exist, proceeding with operation";
            m_pendingBranch.clear();

            // Check if upstream is configured
            GitRepositoryStatus status = m_gitManager->cachedStatus();

            if (isPush) {
                if (status.upstream.isEmpty()) {
                    qDebug() << "[GitChangesPanel] No upstream, calling pushWithUpstream(" << remotes.first() << "," << branch << ")";
                    m_gitManager->pushWithUpstream(remotes.first(), branch);
                } else {
                    qDebug() << "[GitChangesPanel] Has upstream=" << status.upstream << ", calling push()";
                    m_gitManager->push();
                }
            } else {
                // Pull
                if (status.upstream.isEmpty()) {
                    qDebug() << "[GitChangesPanel] No upstream, calling pull(" << remotes.first() << "," << branch << ")";
                    m_gitManager->pull(remotes.first(), branch);
                } else {
                    qDebug() << "[GitChangesPanel] Has upstream=" << status.upstream << ", calling pull()";
                    m_gitManager->pull();
                }
            }
        }
    } else {
        qDebug() << "[GitChangesPanel] No pending operation, ignoring remotesReceived";
    }
}

void GitChangesPanel::onRemoteAdded(bool success, const QString& name, const QString& error)
{
    qDebug() << "[GitChangesPanel] >>> onRemoteAdded: success=" << success << "name=" << name
             << "error=" << error << "pendingBranch=" << m_pendingBranch << "isPush=" << m_pendingIsPush;

    if (success) {
        // Now push/pull with the new remote
        if (!m_pendingBranch.isEmpty()) {
            QString branch = m_pendingBranch;
            bool isPush = m_pendingIsPush;
            m_pendingBranch.clear();

            if (isPush) {
                qDebug() << "[GitChangesPanel] Calling pushWithUpstream(" << name << "," << branch << ")";
                m_gitManager->pushWithUpstream(name, branch);
            } else {
                qDebug() << "[GitChangesPanel] Calling pull(" << name << "," << branch << ")";
                m_gitManager->pull(name, branch);
            }
        } else {
            qDebug() << "[GitChangesPanel] WARNING: Remote added but no pending branch!";
        }
    } else {
        qDebug() << "[GitChangesPanel] Remote add failed, showing error dialog";
        QMessageBox::warning(this, tr("Add Remote Failed"),
                             tr("Failed to add remote '%1':\n%2").arg(name, error));
        m_pendingBranch.clear();
    }
    qDebug() << "[GitChangesPanel] <<< onRemoteAdded complete";
}

void GitChangesPanel::onInitClicked()
{
    qDebug() << "[GitChangesPanel] onInitClicked";

    if (!m_gitManager) {
        return;
    }

    if (m_gitManager->repositoryPath().isEmpty()) {
        QMessageBox::information(this, tr("No Project Open"),
                                 tr("Please open a project first before initializing a Git repository."));
        return;
    }

    // Confirm with user
    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        tr("Initialize Git Repository"),
        tr("Initialize a new Git repository in:\n\n%1\n\nThis will create a .git folder in the project directory.")
            .arg(m_gitManager->repositoryPath()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        m_initRepoButton->setEnabled(false);
        m_initRepoButton->setText(tr("Initializing..."));
        m_gitManager->initRepository();
    }
}

void GitChangesPanel::onInitCompleted(bool success, const QString& error)
{
    qDebug() << "[GitChangesPanel] onInitCompleted: success=" << success << "error=" << error;

    // Reset button state
    m_initRepoButton->setEnabled(true);
    m_initRepoButton->setText(tr("Initialize Repository"));

    if (!success) {
        QMessageBox::warning(this, tr("Initialization Failed"),
                             tr("Failed to initialize Git repository:\n\n%1").arg(error));
    }
    // On success, the repositoryChanged signal will automatically hide the no-repo view
    // and show the main content view. No message box needed - the UI update is sufficient.
}

void GitChangesPanel::onOperationStarted(const QString& operation)
{
    qDebug() << "[GitChangesPanel] onOperationStarted:" << operation;

    // Show operation in progress
    if (m_branchLabel) {
        m_branchLabel->setStyleSheet("font-weight: bold; color: #dcdcaa;");
        m_branchLabel->setText(operation);
    }

    // Disable buttons during operation
    m_fetchButton->setEnabled(false);
    m_pullButton->setEnabled(false);
    m_pushButton->setEnabled(false);
}

void GitChangesPanel::onFetchCompleted(bool success, const QString& error)
{
    qDebug() << "[GitChangesPanel] onFetchCompleted: success=" << success << "error=" << error;

    // Re-enable buttons
    m_fetchButton->setEnabled(true);
    m_pullButton->setEnabled(true);
    m_pushButton->setEnabled(true);

    if (success) {
        showStatusMessage(tr("Fetch completed successfully"), false, 3000);
    } else {
        showStatusMessage(tr("Fetch failed: %1").arg(error), true);
    }
}

void GitChangesPanel::onPullCompleted(bool success, const QString& error)
{
    qDebug() << "[GitChangesPanel] onPullCompleted: success=" << success << "error=" << error;

    // Re-enable buttons
    m_fetchButton->setEnabled(true);
    m_pullButton->setEnabled(true);
    m_pushButton->setEnabled(true);

    if (success) {
        showStatusMessage(tr("Pull completed successfully"), false, 3000);
    } else {
        showStatusMessage(tr("Pull failed: %1").arg(error), true);
    }
}

void GitChangesPanel::onPushCompleted(bool success, const QString& error)
{
    qDebug() << "[GitChangesPanel] onPushCompleted: success=" << success << "error=" << error;

    // Re-enable buttons
    m_fetchButton->setEnabled(true);
    m_pullButton->setEnabled(true);
    m_pushButton->setEnabled(true);

    if (success) {
        showStatusMessage(tr("Push completed successfully"), false, 3000);
    } else {
        showStatusMessage(tr("Push failed: %1").arg(error), true);
    }
}

} // namespace XXMLStudio
