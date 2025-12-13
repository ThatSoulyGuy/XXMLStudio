#ifndef DEPENDENCYDIALOG_H
#define DEPENDENCYDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include "project/Project.h"

namespace XXMLStudio {

class DependencyManager;

/**
 * Main dialog for managing project dependencies.
 * Shows a table of dependencies with Add/Edit/Remove functionality.
 */
class DependencyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DependencyDialog(Project* project, DependencyManager* depManager, QWidget* parent = nullptr);
    ~DependencyDialog();

public slots:
    void accept() override;

private slots:
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void onUpdateClicked();
    void onRefreshClicked();
    void onSelectionChanged();
    void onResolutionProgress(const QString& message);
    void onDependencyResolved(const QString& name, const QString& path);
    void onResolutionFinished(bool success);
    void onResolutionError(const QString& message);

private:
    void setupUi();
    void populateTable();
    void updateButtons();
    void updateDependencyStatus(int row, const QString& status, const QColor& color = QColor());
    int findTableRowByName(const QString& name) const;

    // UI Elements
    QTableWidget* m_table = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_removeButton = nullptr;
    QPushButton* m_updateButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Data
    Project* m_project = nullptr;
    DependencyManager* m_depManager = nullptr;
    QList<Dependency> m_pendingDependencies; // Working copy of dependencies
    bool m_isRefreshing = false;
};

} // namespace XXMLStudio

#endif // DEPENDENCYDIALOG_H
