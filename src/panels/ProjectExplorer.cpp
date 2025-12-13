#include "ProjectExplorer.h"
#include "GitFileDecorator.h"

#include <QClipboard>
#include <QDir>
#include <QGuiApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QUrl>
#include <QDrag>
#include <QPainter>
#include <QPixmap>
#include <QApplication>
#include <QTimer>
#include <QAbstractProxyModel>

namespace XXMLStudio {

// Helper to get QFileSystemModel from a possibly proxied model
static QFileSystemModel* getFileSystemModel(QAbstractItemModel* model)
{
    QFileSystemModel* fsModel = qobject_cast<QFileSystemModel*>(model);
    if (fsModel) {
        return fsModel;
    }

    // Try to get it through a proxy
    QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(model);
    if (proxyModel) {
        return qobject_cast<QFileSystemModel*>(proxyModel->sourceModel());
    }

    return nullptr;
}

// DragDropTreeView implementation
DragDropTreeView::DragDropTreeView(QWidget* parent)
    : QTreeView(parent)
{
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);

    // Setup highlight animation
    m_highlightAnimation = new QPropertyAnimation(this, QByteArray());
    m_highlightAnimation->setDuration(150);
    m_highlightAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_highlightAnimation, &QPropertyAnimation::valueChanged, [this](const QVariant& value) {
        m_dropHighlightOpacity = value.toReal();
        viewport()->update();
    });
}

void DragDropTreeView::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList indexes = selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }

    // Get the first selected index for the drag pixmap
    QModelIndex index = indexes.first();

    // Create mime data with file URLs
    QMimeData* mimeData = model()->mimeData(indexes);
    if (!mimeData) {
        return;
    }

    // Create a custom drag pixmap
    QRect itemRect = visualRect(index);
    QPixmap pixmap(itemRect.size() * devicePixelRatio());
    pixmap.setDevicePixelRatio(devicePixelRatio());
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw rounded background with transparency
    QColor bgColor = palette().highlight().color();
    bgColor.setAlpha(200);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRect(0, 0, itemRect.width(), itemRect.height()), 6, 6);

    // Draw the item
    QStyleOptionViewItem option;
    initViewItemOption(&option);
    option.rect = QRect(0, 0, itemRect.width(), itemRect.height());
    option.state |= QStyle::State_Selected;
    itemDelegate()->paint(&painter, option, index);

    // Draw item count badge if multiple items selected
    int itemCount = 0;
    for (const QModelIndex& idx : indexes) {
        if (idx.column() == 0) itemCount++;
    }
    if (itemCount > 1) {
        QString countText = QString::number(itemCount);
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(9);
        painter.setFont(font);

        QFontMetrics fm(font);
        int badgeWidth = qMax(20, fm.horizontalAdvance(countText) + 10);
        int badgeHeight = 18;
        QRect badgeRect(itemRect.width() - badgeWidth - 4, 4, badgeWidth, badgeHeight);

        // Badge background
        painter.setBrush(QColor(255, 100, 100));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(badgeRect, 9, 9);

        // Badge text
        painter.setPen(Qt::white);
        painter.drawText(badgeRect, Qt::AlignCenter, countText);
    }

    painter.end();

    // Create and execute drag
    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(20, pixmap.height() / (2 * devicePixelRatio())));

    // Execute with fade effect
    drag->exec(supportedActions, Qt::MoveAction);

    // Clear any remaining highlight
    clearDropAnimation();
}

void DragDropTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        QTreeView::dragEnterEvent(event);
    }
}

void DragDropTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        QModelIndex index = indexAt(event->position().toPoint());
        if (index.isValid()) {
            // Animate highlight on new target
            if (index != m_currentDropTarget) {
                m_currentDropTarget = index;
                animateDropTarget(index);
            }
            event->acceptProposedAction();
        } else {
            clearDropAnimation();
            event->ignore();
        }
    } else {
        QTreeView::dragMoveEvent(event);
    }
}

void DragDropTreeView::dropEvent(QDropEvent* event)
{
    // Store the target for animation
    QModelIndex targetIndex = indexAt(event->position().toPoint());

    if (!event->mimeData()->hasUrls()) {
        clearDropAnimation();
        QTreeView::dropEvent(event);
        return;
    }

    if (!targetIndex.isValid()) {
        clearDropAnimation();
        event->ignore();
        return;
    }

    QFileSystemModel* fsModel = getFileSystemModel(model());
    if (!fsModel) {
        clearDropAnimation();
        event->ignore();
        return;
    }

    // Map index through proxy if needed
    QModelIndex sourceIndex = targetIndex;
    QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(model());
    if (proxyModel) {
        sourceIndex = proxyModel->mapToSource(targetIndex);
    }

    // Flash animation on successful drop
    m_animatingIndex = targetIndex;
    m_highlightAnimation->stop();
    m_highlightAnimation->setStartValue(0.6);
    m_highlightAnimation->setEndValue(0.0);
    m_highlightAnimation->setDuration(300);
    m_highlightAnimation->start();

    // Determine target directory (use source index for file system operations)
    QString targetPath = fsModel->filePath(sourceIndex);
    QString targetDir;
    if (fsModel->isDir(sourceIndex)) {
        targetDir = targetPath;
    } else {
        targetDir = fsModel->fileInfo(sourceIndex).absolutePath();
    }

    // Process dropped files
    const QList<QUrl>& urls = event->mimeData()->urls();
    for (const QUrl& url : urls) {
        if (url.isLocalFile()) {
            QString sourcePath = url.toLocalFile();
            // Don't drop onto itself or its parent
            QFileInfo sourceInfo(sourcePath);
            if (sourceInfo.absolutePath() != targetDir) {
                emit itemDropped(sourcePath, targetDir);
            }
        }
    }

    m_currentDropTarget = QModelIndex();
    event->acceptProposedAction();
}

void DragDropTreeView::paintEvent(QPaintEvent* event)
{
    QTreeView::paintEvent(event);

    // Draw drop target highlight animation
    if (m_animatingIndex.isValid() && m_dropHighlightOpacity > 0.01) {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        QRect rect = visualRect(m_animatingIndex);
        if (rect.isValid()) {
            QColor highlightColor = palette().highlight().color();
            highlightColor.setAlphaF(m_dropHighlightOpacity);

            painter.setBrush(highlightColor);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(rect.adjusted(-2, -1, 2, 1), 4, 4);
        }
    }
}

void DragDropTreeView::animateDropTarget(const QModelIndex& index)
{
    m_animatingIndex = index;
    m_highlightAnimation->stop();
    m_highlightAnimation->setStartValue(m_dropHighlightOpacity);
    m_highlightAnimation->setEndValue(0.3);
    m_highlightAnimation->setDuration(150);
    m_highlightAnimation->start();
}

void DragDropTreeView::clearDropAnimation()
{
    m_currentDropTarget = QModelIndex();
    if (m_dropHighlightOpacity > 0.01) {
        m_highlightAnimation->stop();
        m_highlightAnimation->setStartValue(m_dropHighlightOpacity);
        m_highlightAnimation->setEndValue(0.0);
        m_highlightAnimation->setDuration(100);
        m_highlightAnimation->start();
    }
}

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

    // Tree view with drag and drop support
    m_treeView = new DragDropTreeView(this);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_treeView->setSortingEnabled(true);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    // Disable double-click to edit - only allow editing via F2 or context menu
    m_treeView->setEditTriggers(QAbstractItemView::EditKeyPressed);
    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    connect(m_treeView, &DragDropTreeView::itemDropped,
            this, &ProjectExplorer::onItemDropped);
}

void ProjectExplorer::setupContextMenu()
{
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            [this](const QPoint& pos) {
        QMenu menu(this);

        QModelIndex proxyIndex = m_treeView->indexAt(pos);

        // Map through proxy if needed
        QModelIndex index = proxyIndex;
        if (m_gitDecorator && proxyIndex.isValid()) {
            index = m_gitDecorator->mapToSource(proxyIndex);
        }

        QString path = m_model->filePath(index);
        QString targetDir = m_rootPath;

        if (index.isValid()) {
            if (m_model->isDir(index)) {
                targetDir = path;
            } else {
                targetDir = m_model->fileInfo(index).absolutePath();
            }
        }

        // New File
        QAction* newFileAction = menu.addAction(tr("New File..."));
        newFileAction->setShortcut(QKeySequence::New);
        connect(newFileAction, &QAction::triggered, [this, targetDir]() {
            bool ok;
            QString name = QInputDialog::getText(this, tr("New File"),
                tr("File name:"), QLineEdit::Normal, "NewFile.XXML", &ok);
            if (ok && !name.isEmpty()) {
                QString filePath = targetDir + "/" + name;
                QFile file(filePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.close();
                    emit fileDoubleClicked(filePath);  // Open the new file
                }
            }
        });

        // New Folder
        QAction* newFolderAction = menu.addAction(tr("New Folder..."));
        connect(newFolderAction, &QAction::triggered, [this, targetDir]() {
            bool ok;
            QString name = QInputDialog::getText(this, tr("New Folder"),
                tr("Folder name:"), QLineEdit::Normal, "NewFolder", &ok);
            if (ok && !name.isEmpty()) {
                QDir(targetDir).mkdir(name);
            }
        });

        menu.addSeparator();

        // Open File (trigger file dialog)
        QAction* openAction = menu.addAction(tr("Open File..."));
        openAction->setShortcut(QKeySequence::Open);
        connect(openAction, &QAction::triggered, [this]() {
            emit openFileRequested();
        });

        // Save current file
        QAction* saveAction = menu.addAction(tr("Save"));
        saveAction->setShortcut(QKeySequence::Save);
        connect(saveAction, &QAction::triggered, [this]() {
            emit saveFileRequested();
        });

        if (index.isValid()) {
            menu.addSeparator();

            // Rename - use proxyIndex for edit since that's what the view needs
            QAction* renameAction = menu.addAction(tr("Rename"));
            renameAction->setShortcut(QKeySequence("F2"));
            connect(renameAction, &QAction::triggered, [this, proxyIndex]() {
                m_treeView->edit(proxyIndex);
            });

            // Delete - use source index for model operations
            QAction* deleteAction = menu.addAction(tr("Delete"));
            deleteAction->setShortcut(QKeySequence::Delete);
            connect(deleteAction, &QAction::triggered, [this, index, path]() {
                QString name = m_model->fileName(index);
                int result = QMessageBox::question(this, tr("Delete"),
                    tr("Are you sure you want to delete '%1'?").arg(name),
                    QMessageBox::Yes | QMessageBox::No);
                if (result == QMessageBox::Yes) {
                    m_model->remove(index);
                }
            });

            menu.addSeparator();

            // Copy Path
            QAction* copyPathAction = menu.addAction(tr("Copy Path"));
            connect(copyPathAction, &QAction::triggered, [path]() {
                QGuiApplication::clipboard()->setText(path);
            });

            // Set as Compilation Entrypoint (only for XXML files)
            if (!m_model->isDir(index)) {
                QString fileName = m_model->fileName(index).toLower();
                if (fileName.endsWith(".xxml")) {
                    menu.addSeparator();
                    QAction* setEntrypointAction = menu.addAction(tr("Set as Compilation Entrypoint"));
                    connect(setEntrypointAction, &QAction::triggered, [this, path]() {
                        emit setCompilationEntrypointRequested(path);
                    });
                }
            }
        }

        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    });
}

void ProjectExplorer::setRootPath(const QString& path)
{
    m_rootPath = path;
    m_model->setRootPath(path);

    if (m_gitDecorator) {
        // Update decorator root path
        m_gitDecorator->setRootPath(path);

        // Map through proxy
        QModelIndex sourceIndex = m_model->index(path);
        QModelIndex proxyIndex = m_gitDecorator->mapFromSource(sourceIndex);
        m_treeView->setRootIndex(proxyIndex);
    } else {
        m_treeView->setRootIndex(m_model->index(path));
    }
}

QString ProjectExplorer::rootPath() const
{
    return m_rootPath;
}

void ProjectExplorer::setGitFileDecorator(GitFileDecorator* decorator)
{
    m_gitDecorator = decorator;
    if (m_gitDecorator) {
        m_gitDecorator->setSourceModel(m_model);
        m_gitDecorator->setRootPath(m_rootPath);
        m_treeView->setModel(m_gitDecorator);

        // Update root index through the proxy
        if (!m_rootPath.isEmpty()) {
            QModelIndex sourceIndex = m_model->index(m_rootPath);
            QModelIndex proxyIndex = m_gitDecorator->mapFromSource(sourceIndex);
            m_treeView->setRootIndex(proxyIndex);
        }

        // Hide columns except name
        for (int i = 1; i < m_gitDecorator->columnCount(); ++i) {
            m_treeView->hideColumn(i);
        }
    }
}

void ProjectExplorer::onItemDoubleClicked(const QModelIndex& index)
{
    // Map through proxy if needed
    QModelIndex sourceIndex = index;
    if (m_gitDecorator) {
        sourceIndex = m_gitDecorator->mapToSource(index);
    }

    if (!m_model->isDir(sourceIndex)) {
        QString path = m_model->filePath(sourceIndex);
        emit fileDoubleClicked(path);
    }
}

void ProjectExplorer::onItemClicked(const QModelIndex& index)
{
    // Map through proxy if needed
    QModelIndex sourceIndex = index;
    if (m_gitDecorator) {
        sourceIndex = m_gitDecorator->mapToSource(index);
    }

    QString path = m_model->filePath(sourceIndex);
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

void ProjectExplorer::onItemDropped(const QString& sourcePath, const QString& targetDir)
{
    QFileInfo sourceInfo(sourcePath);
    QString destPath = targetDir + "/" + sourceInfo.fileName();

    // Check if destination already exists
    if (QFileInfo::exists(destPath)) {
        int result = QMessageBox::question(this, tr("File Exists"),
            tr("'%1' already exists in the destination folder. Do you want to replace it?")
                .arg(sourceInfo.fileName()),
            QMessageBox::Yes | QMessageBox::No);
        if (result != QMessageBox::Yes) {
            return;
        }
        // Remove existing file/folder
        if (QFileInfo(destPath).isDir()) {
            QDir(destPath).removeRecursively();
        } else {
            QFile::remove(destPath);
        }
    }

    if (!moveFileOrFolder(sourcePath, targetDir)) {
        QMessageBox::warning(this, tr("Move Failed"),
            tr("Failed to move '%1' to '%2'.")
                .arg(sourceInfo.fileName())
                .arg(targetDir));
    }
}

bool ProjectExplorer::moveFileOrFolder(const QString& sourcePath, const QString& targetDir)
{
    QFileInfo sourceInfo(sourcePath);
    QString destPath = targetDir + "/" + sourceInfo.fileName();

    if (sourceInfo.isDir()) {
        // Move directory
        QDir dir;
        return dir.rename(sourcePath, destPath);
    } else {
        // Move file
        return QFile::rename(sourcePath, destPath);
    }
}

} // namespace XXMLStudio
