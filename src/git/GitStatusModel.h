#ifndef GITSTATUSMODEL_H
#define GITSTATUSMODEL_H

#include <QAbstractItemModel>
#include <QIcon>
#include "GitTypes.h"

namespace XXMLStudio {

/**
 * Tree model for displaying Git status in GitChangesPanel.
 * Structure:
 *   - Staged Changes (N)
 *     - file1.cpp
 *     - file2.h
 *   - Changes (N)
 *     - file3.cpp
 *   - Untracked Files (N)
 *     - file4.txt
 */
class GitStatusModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum class Section {
        Staged = 0,
        Unstaged = 1,
        Untracked = 2,
        SectionCount = 3
    };

    enum Role {
        PathRole = Qt::UserRole + 1,
        IndexStatusRole,
        WorkTreeStatusRole,
        SectionRole,
        IsHeaderRole
    };

    explicit GitStatusModel(QObject* parent = nullptr);
    ~GitStatusModel() override;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Data management
    void setStatus(const GitRepositoryStatus& status);
    void clear();

    // Get entries by section
    QList<GitStatusEntry> stagedEntries() const { return m_stagedEntries; }
    QList<GitStatusEntry> unstagedEntries() const { return m_unstagedEntries; }
    QList<GitStatusEntry> untrackedEntries() const { return m_untrackedEntries; }

    // Get entry at index
    GitStatusEntry entryAt(const QModelIndex& index) const;
    Section sectionAt(const QModelIndex& index) const;
    bool isHeader(const QModelIndex& index) const;

    // Get paths for selection
    QStringList pathsForIndices(const QModelIndexList& indices) const;

private:
    QString sectionTitle(Section section) const;
    QIcon statusIcon(GitFileStatus status) const;
    QColor statusColor(GitFileStatus status) const;

    QList<GitStatusEntry> m_stagedEntries;
    QList<GitStatusEntry> m_unstagedEntries;
    QList<GitStatusEntry> m_untrackedEntries;
};

} // namespace XXMLStudio

#endif // GITSTATUSMODEL_H
