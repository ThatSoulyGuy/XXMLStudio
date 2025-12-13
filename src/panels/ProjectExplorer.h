#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDropEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

namespace XXMLStudio {

class GitFileDecorator;

/**
 * Custom tree view that handles drag and drop for file operations.
 */
class DragDropTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit DragDropTreeView(QWidget* parent = nullptr);

signals:
    void itemDropped(const QString& sourcePath, const QString& targetDir);
    void dropAnimationStarted(const QModelIndex& targetIndex);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void animateDropTarget(const QModelIndex& index);
    void clearDropAnimation();

    QModelIndex m_currentDropTarget;
    QModelIndex m_animatingIndex;
    qreal m_dropHighlightOpacity = 0.0;
    QPropertyAnimation* m_highlightAnimation = nullptr;
};

/**
 * Project explorer panel showing project files in a tree view.
 */
class ProjectExplorer : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectExplorer(QWidget* parent = nullptr);
    ~ProjectExplorer();

    void setRootPath(const QString& path);
    QString rootPath() const;

    void setGitFileDecorator(GitFileDecorator* decorator);
    QFileSystemModel* fileSystemModel() const { return m_model; }

signals:
    void fileDoubleClicked(const QString& path);
    void fileSelected(const QString& path);
    void newFileRequested(const QString& directory);
    void newFolderRequested(const QString& directory);
    void openFileRequested();
    void saveFileRequested();
    void setCompilationEntrypointRequested(const QString& path);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onItemClicked(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);
    void onItemDropped(const QString& sourcePath, const QString& targetDir);

private:
    void setupUi();
    void setupContextMenu();
    bool moveFileOrFolder(const QString& sourcePath, const QString& targetDir);

    QVBoxLayout* m_layout = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    DragDropTreeView* m_treeView = nullptr;
    QFileSystemModel* m_model = nullptr;
    GitFileDecorator* m_gitDecorator = nullptr;
    QString m_rootPath;
};

} // namespace XXMLStudio

#endif // PROJECTEXPLORER_H
