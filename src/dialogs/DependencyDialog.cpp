#include "DependencyDialog.h"
#include "AddDependencyDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QTimer>
#include <QPointer>

#include "project/DependencyManager.h"

namespace XXMLStudio {

DependencyDialog::DependencyDialog(Project* project, DependencyManager* depManager, QWidget* parent)
    : QDialog(parent)
    , m_project(project)
    , m_depManager(depManager)
{
    setWindowTitle(tr("Manage Dependencies"));
    setMinimumSize(900, 500);
    setupUi();

    // Copy current dependencies to working list
    m_pendingDependencies = m_project->dependencies();
    populateTable();

    // Connect to DependencyManager signals for refresh
    connect(m_depManager, &DependencyManager::resolutionProgress,
            this, &DependencyDialog::onResolutionProgress);
    connect(m_depManager, &DependencyManager::dependencyResolved,
            this, &DependencyDialog::onDependencyResolved);
    connect(m_depManager, &DependencyManager::resolutionFinished,
            this, &DependencyDialog::onResolutionFinished);
    connect(m_depManager, &DependencyManager::error,
            this, &DependencyDialog::onResolutionError);
}

DependencyDialog::~DependencyDialog()
{
}

void DependencyDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title
    QLabel* titleLabel = new QLabel(tr("Project Dependencies"), this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    // Description
    QLabel* descLabel = new QLabel(
        tr("Dependencies are Git repositories containing XXML library projects. "
           "They must have a .xxmlp file with Type = Library."),
        this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888; margin-bottom: 10px;");
    mainLayout->addWidget(descLabel);

    // Main content layout (table + buttons)
    QHBoxLayout* contentLayout = new QHBoxLayout();

    // Table with 6 columns
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("Name"),
        tr("URL"),
        tr("Tag"),
        tr("Commit"),
        tr("Local Path"),
        tr("Status")
    });
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);

    contentLayout->addWidget(m_table, 1);

    // Action buttons (vertical layout on right side)
    QVBoxLayout* buttonLayout = new QVBoxLayout();

    m_addButton = new QPushButton(tr("Add..."), this);
    buttonLayout->addWidget(m_addButton);

    m_editButton = new QPushButton(tr("Edit..."), this);
    m_editButton->setEnabled(false);
    buttonLayout->addWidget(m_editButton);

    m_removeButton = new QPushButton(tr("Remove"), this);
    m_removeButton->setEnabled(false);
    buttonLayout->addWidget(m_removeButton);

    buttonLayout->addSpacing(20);

    m_updateButton = new QPushButton(tr("Update"), this);
    m_updateButton->setToolTip(tr("Re-download the selected dependency from Git"));
    m_updateButton->setEnabled(false);
    buttonLayout->addWidget(m_updateButton);

    m_refreshButton = new QPushButton(tr("Refresh All"), this);
    buttonLayout->addWidget(m_refreshButton);

    buttonLayout->addStretch();

    contentLayout->addLayout(buttonLayout);

    mainLayout->addLayout(contentLayout, 1);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_statusLabel);

    // Dialog buttons
    QHBoxLayout* dialogButtonLayout = new QHBoxLayout();
    dialogButtonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    dialogButtonLayout->addWidget(m_cancelButton);

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    dialogButtonLayout->addWidget(m_okButton);

    mainLayout->addLayout(dialogButtonLayout);

    // Connections
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &DependencyDialog::onSelectionChanged);
    connect(m_addButton, &QPushButton::clicked, this, &DependencyDialog::onAddClicked);
    connect(m_editButton, &QPushButton::clicked, this, &DependencyDialog::onEditClicked);
    connect(m_removeButton, &QPushButton::clicked, this, &DependencyDialog::onRemoveClicked);
    connect(m_updateButton, &QPushButton::clicked, this, &DependencyDialog::onUpdateClicked);
    connect(m_refreshButton, &QPushButton::clicked, this, &DependencyDialog::onRefreshClicked);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void DependencyDialog::populateTable()
{
    m_table->setRowCount(0);

    for (const Dependency& dep : m_pendingDependencies) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(dep.name));
        m_table->setItem(row, 1, new QTableWidgetItem(dep.gitUrl));
        m_table->setItem(row, 2, new QTableWidgetItem(dep.tag.isEmpty() ? "default" : dep.tag));

        // Short commit hash (first 8 characters)
        QString shortHash = dep.commitHash.left(8);
        m_table->setItem(row, 3, new QTableWidgetItem(shortHash.isEmpty() ? "-" : shortHash));

        m_table->setItem(row, 4, new QTableWidgetItem(dep.localPath.isEmpty() ? "-" : dep.localPath));

        // Determine status
        QString status;
        QColor statusColor;
        if (dep.localPath.isEmpty()) {
            status = tr("Not fetched");
            statusColor = QColor("#ff5555"); // Red
        } else if (m_depManager->isCached(dep.gitUrl, dep.tag)) {
            status = tr("Cached");
            statusColor = QColor("#50fa7b"); // Green
        } else {
            status = tr("Pending");
            statusColor = QColor("#f1fa8c"); // Yellow
        }

        QTableWidgetItem* statusItem = new QTableWidgetItem(status);
        statusItem->setForeground(statusColor);
        m_table->setItem(row, 5, statusItem);
    }

    updateButtons();

    if (m_pendingDependencies.isEmpty()) {
        m_statusLabel->setText(tr("No dependencies. Click 'Add...' to add a dependency."));
    } else {
        m_statusLabel->setText(tr("%1 dependencies").arg(m_pendingDependencies.size()));
    }
}

void DependencyDialog::updateButtons()
{
    bool hasSelection = m_table->currentRow() >= 0;
    m_editButton->setEnabled(hasSelection && !m_isRefreshing);
    m_removeButton->setEnabled(hasSelection && !m_isRefreshing);
    m_updateButton->setEnabled(hasSelection && !m_isRefreshing);
    m_addButton->setEnabled(!m_isRefreshing);
    m_refreshButton->setEnabled(!m_pendingDependencies.isEmpty() && !m_isRefreshing);
}

void DependencyDialog::updateDependencyStatus(int row, const QString& status, const QColor& color)
{
    if (!m_table || row < 0 || row >= m_table->rowCount()) {
        return;
    }
    QTableWidgetItem* statusItem = m_table->item(row, 5);
    if (statusItem) {
        statusItem->setText(status);
        if (color.isValid()) {
            statusItem->setForeground(color);
        }
    }
}

int DependencyDialog::findTableRowByName(const QString& name) const
{
    if (!m_table) {
        return -1;
    }
    for (int row = 0; row < m_table->rowCount(); ++row) {
        QTableWidgetItem* nameItem = m_table->item(row, 0);
        if (nameItem && nameItem->text() == name) {
            return row;
        }
    }
    return -1;
}

void DependencyDialog::onSelectionChanged()
{
    updateButtons();
}

void DependencyDialog::onAddClicked()
{
    AddDependencyDialog addDialog(m_depManager->cacheDir(), this);
    if (addDialog.exec() == QDialog::Accepted) {
        Dependency newDep = addDialog.dependency();

        // Check for duplicate name
        for (const Dependency& dep : m_pendingDependencies) {
            if (dep.name == newDep.name) {
                QMessageBox::warning(this, tr("Duplicate Dependency"),
                    tr("A dependency named '%1' already exists.").arg(newDep.name));
                return;
            }
        }

        // Check for duplicate URL
        for (const Dependency& dep : m_pendingDependencies) {
            if (dep.gitUrl == newDep.gitUrl) {
                QMessageBox::warning(this, tr("Duplicate Dependency"),
                    tr("A dependency with URL '%1' already exists.").arg(newDep.gitUrl));
                return;
            }
        }

        m_pendingDependencies.append(newDep);
        populateTable();
        m_statusLabel->setText(tr("Added dependency: %1").arg(newDep.name));
    }
}

void DependencyDialog::onEditClicked()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_pendingDependencies.size()) {
        return;
    }

    Dependency& dep = m_pendingDependencies[row];

    // For now, editing means re-adding with pre-filled values
    // A full implementation would have a dedicated edit dialog
    AddDependencyDialog editDialog(m_depManager->cacheDir(), this);
    editDialog.setWindowTitle(tr("Edit Dependency"));

    // TODO: Pre-fill the dialog with existing values
    // This would require adding setter methods to AddDependencyDialog

    if (editDialog.exec() == QDialog::Accepted) {
        Dependency newDep = editDialog.dependency();

        // Check for duplicate (excluding current)
        for (int i = 0; i < m_pendingDependencies.size(); ++i) {
            if (i != row && m_pendingDependencies[i].name == newDep.name) {
                QMessageBox::warning(this, tr("Duplicate Dependency"),
                    tr("A dependency named '%1' already exists.").arg(newDep.name));
                return;
            }
        }

        m_pendingDependencies[row] = newDep;
        populateTable();
        m_statusLabel->setText(tr("Updated dependency: %1").arg(newDep.name));
    }
}

void DependencyDialog::onRemoveClicked()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_pendingDependencies.size()) {
        return;
    }

    QString name = m_pendingDependencies[row].name;

    QMessageBox::StandardButton result = QMessageBox::question(this,
        tr("Remove Dependency"),
        tr("Are you sure you want to remove the dependency '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_pendingDependencies.removeAt(row);
        populateTable();
        m_statusLabel->setText(tr("Removed dependency: %1").arg(name));
    }
}

void DependencyDialog::onUpdateClicked()
{
    int row = m_table->currentRow();
    if (row < 0 || row >= m_pendingDependencies.size()) {
        return;
    }

    if (!m_project || !m_depManager) {
        m_statusLabel->setText(tr("Error: Invalid project or dependency manager."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return;
    }

    Dependency& dep = m_pendingDependencies[row];

    // Clear the cache for this dependency to force re-download
    m_depManager->clearCacheFor(dep.gitUrl, dep.tag);

    // Also clear local path since we're re-downloading
    dep.localPath.clear();
    dep.commitHash.clear();

    m_isRefreshing = true;
    updateButtons();
    m_statusLabel->setText(tr("Updating %1...").arg(dep.name));
    m_statusLabel->setStyleSheet("color: #888;");

    // Update status in table
    updateDependencyStatus(row, tr("Updating..."), QColor("#f1fa8c"));

    // Apply changes to project temporarily and resolve
    QList<Dependency> originalDeps = m_project->dependencies();
    for (const Dependency& d : originalDeps) {
        m_project->removeDependency(d.name);
    }
    for (const Dependency& d : m_pendingDependencies) {
        m_project->addDependency(d);
    }

    m_depManager->resolveDependencies(m_project);
}

void DependencyDialog::onRefreshClicked()
{
    if (m_pendingDependencies.isEmpty()) {
        return;
    }

    if (!m_project || !m_depManager) {
        m_statusLabel->setText(tr("Error: Invalid project or dependency manager."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return;
    }

    m_isRefreshing = true;
    updateButtons();
    m_statusLabel->setText(tr("Resolving dependencies..."));
    m_statusLabel->setStyleSheet("color: #888;");

    // Update all dependency statuses to "Resolving..."
    if (m_table) {
        for (int i = 0; i < m_table->rowCount(); ++i) {
            updateDependencyStatus(i, tr("Resolving..."), QColor("#f1fa8c"));
        }
    }

    // Trigger resolution through DependencyManager
    // First apply pending changes to project temporarily
    QList<Dependency> originalDeps = m_project->dependencies();
    for (const Dependency& dep : originalDeps) {
        m_project->removeDependency(dep.name);
    }
    for (const Dependency& dep : m_pendingDependencies) {
        m_project->addDependency(dep);
    }

    m_depManager->resolveDependencies(m_project);
}

void DependencyDialog::onResolutionProgress(const QString& message)
{
    m_statusLabel->setText(message);
}

void DependencyDialog::onDependencyResolved(const QString& name, const QString& path)
{
    // Find the dependency in our list and update its path
    for (int i = 0; i < m_pendingDependencies.size(); ++i) {
        if (m_pendingDependencies[i].name == name) {
            m_pendingDependencies[i].localPath = path;
            break;
        }
    }

    // Find the table row by name (not by index - they may not match)
    int row = findTableRowByName(name);
    if (row >= 0) {
        QTableWidgetItem* pathItem = m_table->item(row, 4);
        if (pathItem) {
            pathItem->setText(path);
        }
        updateDependencyStatus(row, tr("Cached"), QColor("#50fa7b"));
    }
}

void DependencyDialog::onResolutionFinished(bool success)
{
    m_isRefreshing = false;
    updateButtons();

    if (success) {
        m_statusLabel->setText(tr("All dependencies resolved successfully."));
        m_statusLabel->setStyleSheet("color: #50fa7b;");

        // Repopulate table with updated m_pendingDependencies (which have resolved paths)
        // Don't fetch from project - it doesn't have the updated local paths
        populateTable();
    } else {
        m_statusLabel->setText(tr("Some dependencies failed to resolve."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
    }

    // Reset status label style after a moment (use QPointer to safely check if dialog still exists)
    QPointer<DependencyDialog> guard(this);
    QTimer::singleShot(3000, this, [guard]() {
        if (guard && guard->m_statusLabel) {
            guard->m_statusLabel->setStyleSheet("color: #888;");
        }
    });
}

void DependencyDialog::onResolutionError(const QString& message)
{
    m_isRefreshing = false;
    updateButtons();
    m_statusLabel->setText(tr("Error: %1").arg(message));
    m_statusLabel->setStyleSheet("color: #ff5555;");
}

void DependencyDialog::accept()
{
    // Apply changes to project
    // First remove all existing dependencies
    QList<Dependency> originalDeps = m_project->dependencies();
    for (const Dependency& dep : originalDeps) {
        m_project->removeDependency(dep.name);
    }

    // Add all pending dependencies
    for (const Dependency& dep : m_pendingDependencies) {
        m_project->addDependency(dep);
    }

    // Save project and lock file
    m_project->save();
    m_project->saveLockFile();

    QDialog::accept();
}

} // namespace XXMLStudio
