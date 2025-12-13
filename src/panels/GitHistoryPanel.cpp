#include "GitHistoryPanel.h"
#include "git/GitManager.h"
#include "core/IconUtils.h"

#include <QHeaderView>
#include <QToolButton>
#include <QHBoxLayout>

namespace XXMLStudio {

GitHistoryPanel::GitHistoryPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

GitHistoryPanel::~GitHistoryPanel()
{
}

void GitHistoryPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // No repo message
    m_noRepoLabel = new QLabel(tr("No Git repository"), this);
    m_noRepoLabel->setAlignment(Qt::AlignCenter);
    m_noRepoLabel->setStyleSheet("color: #888; padding: 20px;");

    // Toolbar with filter
    m_toolbarWidget = new QWidget(this);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(4);

    QToolButton* refreshButton = new QToolButton(this);
    refreshButton->setIcon(IconUtils::loadForDarkBackground(":/icons/Refresh.svg"));
    refreshButton->setToolTip(tr("Refresh history"));
    connect(refreshButton, &QToolButton::clicked, this, &GitHistoryPanel::onRefreshClicked);
    toolbarLayout->addWidget(refreshButton);

    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter commits..."));
    m_filterEdit->setClearButtonEnabled(true);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &GitHistoryPanel::onFilterTextChanged);
    toolbarLayout->addWidget(m_filterEdit, 1);

    m_layout->addWidget(m_toolbarWidget);

    // Table view
    m_tableView = new QTableView(this);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSortingEnabled(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->horizontalHeader()->setStretchLastSection(true);

    // Model
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr("Hash"), tr("Author"), tr("Date"), tr("Message")});

    // Proxy model for filtering
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);  // Filter all columns

    m_tableView->setModel(m_proxyModel);

    // Column widths
    m_tableView->setColumnWidth(0, 80);   // Hash
    m_tableView->setColumnWidth(1, 150);  // Author
    m_tableView->setColumnWidth(2, 150);  // Date

    m_layout->addWidget(m_tableView, 1);
    m_layout->addWidget(m_noRepoLabel);

    m_noRepoLabel->setVisible(true);
    m_toolbarWidget->setVisible(false);
    m_tableView->setVisible(false);

    // Connections
    connect(m_tableView, &QTableView::doubleClicked, this, &GitHistoryPanel::onItemDoubleClicked);
}

void GitHistoryPanel::setGitManager(GitManager* manager)
{
    if (m_gitManager) {
        disconnect(m_gitManager, nullptr, this, nullptr);
    }

    m_gitManager = manager;

    if (m_gitManager) {
        connect(m_gitManager, &GitManager::logReceived,
                this, &GitHistoryPanel::onLogReceived);
        connect(m_gitManager, &GitManager::repositoryChanged,
                this, &GitHistoryPanel::onRepositoryChanged);

        bool hasRepo = m_gitManager->isGitRepository();
        m_noRepoLabel->setVisible(!hasRepo);
        m_toolbarWidget->setVisible(hasRepo);
        m_tableView->setVisible(hasRepo);

        if (hasRepo) {
            refresh();
        }
    }
}

void GitHistoryPanel::setFilePath(const QString& path)
{
    m_filePath = path;
    refresh();
}

void GitHistoryPanel::refresh()
{
    if (m_gitManager && m_gitManager->isGitRepository()) {
        m_gitManager->getLog(100, m_filePath);
    }
}

void GitHistoryPanel::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_commits.clear();
}

void GitHistoryPanel::onRepositoryChanged(bool isGitRepo)
{
    m_noRepoLabel->setVisible(!isGitRepo);
    m_toolbarWidget->setVisible(isGitRepo);
    m_tableView->setVisible(isGitRepo);

    if (!isGitRepo) {
        clear();
    } else {
        refresh();
    }
}

void GitHistoryPanel::onLogReceived(const QList<GitCommit>& commits)
{
    m_model->removeRows(0, m_model->rowCount());
    m_commits = commits;

    for (const GitCommit& commit : commits) {
        addCommitToModel(commit);
    }
}

void GitHistoryPanel::addCommitToModel(const GitCommit& commit)
{
    QList<QStandardItem*> row;

    // Hash (short)
    QStandardItem* hashItem = new QStandardItem(commit.shortHash);
    hashItem->setData(commit.hash, Qt::UserRole);  // Store full hash
    hashItem->setToolTip(commit.hash);
    hashItem->setFont(QFont("Consolas", -1));
    row.append(hashItem);

    // Author
    QStandardItem* authorItem = new QStandardItem(commit.author);
    authorItem->setToolTip(commit.authorEmail);
    row.append(authorItem);

    // Date
    QStandardItem* dateItem = new QStandardItem(commit.authorDate.toString("yyyy-MM-dd hh:mm"));
    dateItem->setData(commit.authorDate, Qt::UserRole);  // For sorting
    row.append(dateItem);

    // Message (subject only)
    QStandardItem* messageItem = new QStandardItem(commit.subject);
    if (!commit.body.isEmpty()) {
        messageItem->setToolTip(commit.subject + "\n\n" + commit.body);
    }
    row.append(messageItem);

    m_model->appendRow(row);
}

void GitHistoryPanel::onItemDoubleClicked(const QModelIndex& index)
{
    QModelIndex sourceIndex = m_proxyModel->mapToSource(index);
    int row = sourceIndex.row();

    if (row >= 0 && row < m_commits.size()) {
        emit commitDoubleClicked(m_commits[row]);
    }
}

void GitHistoryPanel::onFilterTextChanged(const QString& text)
{
    m_proxyModel->setFilterRegularExpression(QRegularExpression(
        QRegularExpression::escape(text),
        QRegularExpression::CaseInsensitiveOption));
}

void GitHistoryPanel::onRefreshClicked()
{
    refresh();
}

} // namespace XXMLStudio
