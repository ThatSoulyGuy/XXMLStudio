#ifndef GITSTATUSINDICATOR_H
#define GITSTATUSINDICATOR_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include "git/GitTypes.h"

namespace XXMLStudio {

class GitManager;

/**
 * Status bar widget showing: [branch-icon] branch-name [2↑ 1↓]
 * Clickable to show Git panel.
 */
class GitStatusIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit GitStatusIndicator(QWidget* parent = nullptr);
    ~GitStatusIndicator();

    void setGitManager(GitManager* manager);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private slots:
    void onStatusRefreshed(const GitRepositoryStatus& status);
    void onRepositoryChanged(bool isGitRepo);

private:
    void setupUi();
    void updateDisplay(const GitRepositoryStatus& status);

    GitManager* m_gitManager = nullptr;

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_branchIcon = nullptr;
    QLabel* m_branchLabel = nullptr;
    QLabel* m_syncLabel = nullptr;  // Shows ahead/behind counts
};

} // namespace XXMLStudio

#endif // GITSTATUSINDICATOR_H
