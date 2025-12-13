#include "GitStatusIndicator.h"
#include "git/GitManager.h"
#include "core/IconUtils.h"

#include <QMouseEvent>

namespace XXMLStudio {

GitStatusIndicator::GitStatusIndicator(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

GitStatusIndicator::~GitStatusIndicator()
{
}

void GitStatusIndicator::setupUi()
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(8, 0, 8, 0);
    m_layout->setSpacing(4);

    // Branch icon
    m_branchIcon = new QLabel(this);
    m_branchIcon->setPixmap(IconUtils::loadForDarkBackground(":/icons/Branch.svg").pixmap(14, 14));
    m_layout->addWidget(m_branchIcon);

    // Branch name
    m_branchLabel = new QLabel(this);
    m_branchLabel->setStyleSheet("color: #ccc;");
    m_layout->addWidget(m_branchLabel);

    // Sync status
    m_syncLabel = new QLabel(this);
    m_syncLabel->setStyleSheet("color: #888;");
    m_layout->addWidget(m_syncLabel);

    // Set cursor to indicate clickable
    setCursor(Qt::PointingHandCursor);
    setToolTip(tr("Click to open Git Changes panel"));

    // Initially hidden
    setVisible(false);
}

void GitStatusIndicator::setGitManager(GitManager* manager)
{
    if (m_gitManager) {
        disconnect(m_gitManager, nullptr, this, nullptr);
    }

    m_gitManager = manager;

    if (m_gitManager) {
        connect(m_gitManager, &GitManager::statusRefreshed,
                this, &GitStatusIndicator::onStatusRefreshed);
        connect(m_gitManager, &GitManager::repositoryChanged,
                this, &GitStatusIndicator::onRepositoryChanged);

        setVisible(m_gitManager->isGitRepository());
    }
}

void GitStatusIndicator::onRepositoryChanged(bool isGitRepo)
{
    setVisible(isGitRepo);

    if (!isGitRepo) {
        m_branchLabel->clear();
        m_syncLabel->clear();
    }
}

void GitStatusIndicator::onStatusRefreshed(const GitRepositoryStatus& status)
{
    updateDisplay(status);
}

void GitStatusIndicator::updateDisplay(const GitRepositoryStatus& status)
{
    // Branch name
    QString branchText = status.detachedHead ? tr("HEAD") : status.branch;
    m_branchLabel->setText(branchText);

    // Sync status
    QStringList syncParts;
    if (status.aheadCount > 0) {
        syncParts << QString("%1↑").arg(status.aheadCount);
    }
    if (status.behindCount > 0) {
        syncParts << QString("%1↓").arg(status.behindCount);
    }

    m_syncLabel->setText(syncParts.join(" "));

    // Update tooltip
    QString tooltip = tr("Branch: %1").arg(branchText);
    if (!status.upstream.isEmpty()) {
        tooltip += tr("\nUpstream: %1").arg(status.upstream);
        if (status.aheadCount > 0 || status.behindCount > 0) {
            tooltip += tr("\n%1 ahead, %2 behind").arg(status.aheadCount).arg(status.behindCount);
        }
    }
    tooltip += tr("\n\nClick to open Git Changes panel");
    setToolTip(tooltip);
}

void GitStatusIndicator::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

void GitStatusIndicator::enterEvent(QEnterEvent* event)
{
    m_branchLabel->setStyleSheet("color: #fff;");
    m_syncLabel->setStyleSheet("color: #aaa;");
    QWidget::enterEvent(event);
}

void GitStatusIndicator::leaveEvent(QEvent* event)
{
    m_branchLabel->setStyleSheet("color: #ccc;");
    m_syncLabel->setStyleSheet("color: #888;");
    QWidget::leaveEvent(event);
}

} // namespace XXMLStudio
