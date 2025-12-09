#include "ProjectExplorer.h"

#include <QHeaderView>
#include <QMenu>
#include <QAction>

namespace XXMLStudio {

ProjectExplorer::ProjectExplorer(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupContextMenu();
}

ProjectExplorer::~ProjectExplorer()
{
}

void ProjectExplorer::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Filter edit
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter files..."));
    m_filterEdit->setClearButtonEnabled(true);
    m_layout->addWidget(m_filterEdit);

    // Tree view
    m_treeView = new QTreeView(this);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setSortingEnabled(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_layout->addWidget(m_treeView);

    // File system model
    m_model = new QFileSystemModel(this);
    m_model->setReadOnly(false);
    m_model->setNameFilterDisables(false);

    // Set name filters for XXML files
    QStringList filters;
    filters << "*.xxml" << "*.XXML" << "*.xxmlp" << "*.h" << "*.cpp" << "*.hpp" << "*.md" << "*.txt" << "*.json" << "*.toml";
    m_model->setNameFilters(filters);

    m_treeView->setModel(m_model);

    // Hide columns except name
    for (int i = 1; i < m_model->columnCount(); ++i) {
        m_treeView->hideColumn(i);
    }

    // Connect signals
    connect(m_treeView, &QTreeView::doubleClicked,
            this, &ProjectExplorer::onItemDoubleClicked);
    connect(m_treeView, &QTreeView::clicked,
            this, &ProjectExplorer::onItemClicked);
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &ProjectExplorer::onFilterTextChanged);
}

void ProjectExplorer::setupContextMenu()
{
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            [this](const QPoint& pos) {
        QMenu menu(this);

        QModelIndex index = m_treeView->indexAt(pos);
        QString path = m_model->filePath(index);

        if (index.isValid()) {
            if (m_model->isDir(index)) {
                menu.addAction(tr("New File..."), [this, path]() {
                    // TODO: Implement new file dialog
                });
                menu.addAction(tr("New Folder..."), [this, path]() {
                    // TODO: Implement new folder dialog
                });
                menu.addSeparator();
            }

            menu.addAction(tr("Rename"), [this, index]() {
                m_treeView->edit(index);
            });

            menu.addAction(tr("Delete"), [this, index]() {
                // TODO: Implement delete confirmation
                m_model->remove(index);
            });

            menu.addSeparator();
            menu.addAction(tr("Copy Path"), [path]() {
                // TODO: Copy to clipboard
            });
        } else {
            menu.addAction(tr("New File..."), [this]() {
                // TODO: Implement new file dialog
            });
            menu.addAction(tr("New Folder..."), [this]() {
                // TODO: Implement new folder dialog
            });
        }

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    });
}

void ProjectExplorer::setRootPath(const QString& path)
{
    m_rootPath = path;
    m_model->setRootPath(path);
    m_treeView->setRootIndex(m_model->index(path));
}

QString ProjectExplorer::rootPath() const
{
    return m_rootPath;
}

void ProjectExplorer::onItemDoubleClicked(const QModelIndex& index)
{
    if (!m_model->isDir(index)) {
        QString path = m_model->filePath(index);
        emit fileDoubleClicked(path);
    }
}

void ProjectExplorer::onItemClicked(const QModelIndex& index)
{
    QString path = m_model->filePath(index);
    emit fileSelected(path);
}

void ProjectExplorer::onFilterTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        m_model->setNameFilterDisables(false);
        QStringList filters;
        filters << "*.xxml" << "*.XXML" << "*.xxmlp" << "*.h" << "*.cpp" << "*.hpp" << "*.md" << "*.txt" << "*.json" << "*.toml";
        m_model->setNameFilters(filters);
    } else {
        m_model->setNameFilterDisables(false);
        m_model->setNameFilters(QStringList() << QString("*%1*").arg(text));
    }
}

} // namespace XXMLStudio
