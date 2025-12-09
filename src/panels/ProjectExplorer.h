#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QLineEdit>

namespace XXMLStudio {

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

signals:
    void fileDoubleClicked(const QString& path);
    void fileSelected(const QString& path);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onItemClicked(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);

private:
    void setupUi();
    void setupContextMenu();

    QVBoxLayout* m_layout = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QTreeView* m_treeView = nullptr;
    QFileSystemModel* m_model = nullptr;
    QString m_rootPath;
};

} // namespace XXMLStudio

#endif // PROJECTEXPLORER_H
