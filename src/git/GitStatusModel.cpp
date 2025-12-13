#include "GitStatusModel.h"

#include <QFileInfo>
#include <QColor>
#include <QFont>

namespace XXMLStudio {

GitStatusModel::GitStatusModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

GitStatusModel::~GitStatusModel()
{
}

QModelIndex GitStatusModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) {
        return QModelIndex();
    }

    if (!parent.isValid()) {
        // Root level - section headers
        if (row >= 0 && row < static_cast<int>(Section::SectionCount)) {
            // Use quintptr to store section index (offset by 1 to distinguish from child items)
            return createIndex(row, column, quintptr(row + 1));
        }
    } else {
        // Child level - file entries
        Section section = static_cast<Section>(parent.row());
        int count = 0;
        switch (section) {
        case Section::Staged: count = m_stagedEntries.size(); break;
        case Section::Unstaged: count = m_unstagedEntries.size(); break;
        case Section::Untracked: count = m_untrackedEntries.size(); break;
        default: break;
        }

        if (row >= 0 && row < count) {
            // Store section in internal ID (0 = child of section 0, etc.)
            // We use section * 10000 + row to encode both
            return createIndex(row, column, quintptr((static_cast<int>(section) + 1) * 10000 + row));
        }
    }

    return QModelIndex();
}

QModelIndex GitStatusModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    quintptr id = child.internalId();

    // If ID is 1, 2, or 3, it's a section header (root level)
    if (id >= 1 && id <= 3) {
        return QModelIndex();
    }

    // Otherwise it's a file entry - extract section from ID
    int section = (id / 10000) - 1;
    if (section >= 0 && section < static_cast<int>(Section::SectionCount)) {
        return createIndex(section, 0, quintptr(section + 1));
    }

    return QModelIndex();
}

int GitStatusModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        // Root level - 3 sections
        return static_cast<int>(Section::SectionCount);
    }

    // Check if parent is a section header
    quintptr id = parent.internalId();
    if (id >= 1 && id <= 3) {
        Section section = static_cast<Section>(id - 1);
        switch (section) {
        case Section::Staged: return m_stagedEntries.size();
        case Section::Unstaged: return m_unstagedEntries.size();
        case Section::Untracked: return m_untrackedEntries.size();
        default: return 0;
        }
    }

    // File entries have no children
    return 0;
}

int GitStatusModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant GitStatusModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    quintptr id = index.internalId();

    // Section header
    if (id >= 1 && id <= 3) {
        Section section = static_cast<Section>(id - 1);

        switch (role) {
        case Qt::DisplayRole:
            return sectionTitle(section);
        case IsHeaderRole:
            return true;
        case SectionRole:
            return static_cast<int>(section);
        case Qt::FontRole: {
            QFont font;
            font.setBold(true);
            return font;
        }
        default:
            return QVariant();
        }
    }

    // File entry
    int section = (id / 10000) - 1;
    int row = id % 10000;

    const QList<GitStatusEntry>* entries = nullptr;
    GitFileStatus displayStatus = GitFileStatus::Unmodified;

    switch (static_cast<Section>(section)) {
    case Section::Staged:
        if (row < m_stagedEntries.size()) {
            entries = &m_stagedEntries;
            displayStatus = m_stagedEntries[row].indexStatus;
        }
        break;
    case Section::Unstaged:
        if (row < m_unstagedEntries.size()) {
            entries = &m_unstagedEntries;
            displayStatus = m_unstagedEntries[row].workTreeStatus;
        }
        break;
    case Section::Untracked:
        if (row < m_untrackedEntries.size()) {
            entries = &m_untrackedEntries;
            displayStatus = GitFileStatus::Untracked;
        }
        break;
    default:
        break;
    }

    if (!entries || row >= entries->size()) {
        return QVariant();
    }

    const GitStatusEntry& entry = entries->at(row);

    switch (role) {
    case Qt::DisplayRole: {
        // Show filename with status prefix
        QString filename = QFileInfo(entry.path).fileName();
        QChar statusChar = GitStatusEntry::statusChar(displayStatus);
        return QString("%1  %2").arg(statusChar).arg(entry.path);
    }
    case Qt::ToolTipRole:
        return QString("%1 - %2").arg(entry.path, GitStatusEntry::statusString(displayStatus));

    case Qt::ForegroundRole:
        return statusColor(displayStatus);

    case Qt::DecorationRole:
        return statusIcon(displayStatus);

    case PathRole:
        return entry.path;

    case IndexStatusRole:
        return static_cast<int>(entry.indexStatus);

    case WorkTreeStatusRole:
        return static_cast<int>(entry.workTreeStatus);

    case SectionRole:
        return section;

    case IsHeaderRole:
        return false;

    default:
        return QVariant();
    }
}

Qt::ItemFlags GitStatusModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags flags = Qt::ItemIsEnabled;

    // File entries are selectable
    if (!isHeader(index)) {
        flags |= Qt::ItemIsSelectable;
    }

    return flags;
}

void GitStatusModel::setStatus(const GitRepositoryStatus& status)
{
    beginResetModel();

    m_stagedEntries.clear();
    m_unstagedEntries.clear();
    m_untrackedEntries.clear();

    for (const GitStatusEntry& entry : status.entries) {
        if (entry.isUntracked()) {
            m_untrackedEntries.append(entry);
        } else {
            if (entry.isStaged()) {
                m_stagedEntries.append(entry);
            }
            if (entry.isUnstaged()) {
                m_unstagedEntries.append(entry);
            }
        }
    }

    endResetModel();
}

void GitStatusModel::clear()
{
    beginResetModel();
    m_stagedEntries.clear();
    m_unstagedEntries.clear();
    m_untrackedEntries.clear();
    endResetModel();
}

GitStatusEntry GitStatusModel::entryAt(const QModelIndex& index) const
{
    if (!index.isValid() || isHeader(index)) {
        return GitStatusEntry();
    }

    quintptr id = index.internalId();
    int section = (id / 10000) - 1;
    int row = id % 10000;

    switch (static_cast<Section>(section)) {
    case Section::Staged:
        if (row < m_stagedEntries.size()) return m_stagedEntries[row];
        break;
    case Section::Unstaged:
        if (row < m_unstagedEntries.size()) return m_unstagedEntries[row];
        break;
    case Section::Untracked:
        if (row < m_untrackedEntries.size()) return m_untrackedEntries[row];
        break;
    default:
        break;
    }

    return GitStatusEntry();
}

GitStatusModel::Section GitStatusModel::sectionAt(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Section::Staged;
    }

    quintptr id = index.internalId();

    if (id >= 1 && id <= 3) {
        return static_cast<Section>(id - 1);
    }

    int section = (id / 10000) - 1;
    return static_cast<Section>(section);
}

bool GitStatusModel::isHeader(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return false;
    }

    quintptr id = index.internalId();
    return (id >= 1 && id <= 3);
}

QStringList GitStatusModel::pathsForIndices(const QModelIndexList& indices) const
{
    QStringList paths;
    for (const QModelIndex& index : indices) {
        if (!isHeader(index)) {
            GitStatusEntry entry = entryAt(index);
            if (!entry.path.isEmpty()) {
                paths.append(entry.path);
            }
        }
    }
    return paths;
}

QString GitStatusModel::sectionTitle(Section section) const
{
    switch (section) {
    case Section::Staged:
        return tr("Staged Changes (%1)").arg(m_stagedEntries.size());
    case Section::Unstaged:
        return tr("Changes (%1)").arg(m_unstagedEntries.size());
    case Section::Untracked:
        return tr("Untracked Files (%1)").arg(m_untrackedEntries.size());
    default:
        return QString();
    }
}

QIcon GitStatusModel::statusIcon(GitFileStatus status) const
{
    // Return empty icon - we'll use text coloring instead
    // Could be extended to use actual icons if desired
    Q_UNUSED(status)
    return QIcon();
}

QColor GitStatusModel::statusColor(GitFileStatus status) const
{
    switch (status) {
    case GitFileStatus::Modified:
        return QColor("#e2c08d");  // Orange/yellow for modified
    case GitFileStatus::Added:
        return QColor("#73c991");  // Green for added
    case GitFileStatus::Deleted:
        return QColor("#f14c4c");  // Red for deleted
    case GitFileStatus::Renamed:
        return QColor("#4fc1ff");  // Cyan for renamed
    case GitFileStatus::Untracked:
        return QColor("#888888");  // Gray for untracked
    case GitFileStatus::Conflicted:
        return QColor("#f14c4c");  // Red for conflicts
    default:
        return QColor("#cccccc");  // Default light gray
    }
}

} // namespace XXMLStudio
