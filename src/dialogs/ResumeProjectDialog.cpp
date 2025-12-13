#include "ResumeProjectDialog.h"

#include <QFileInfo>
#include <QHBoxLayout>

namespace XXMLStudio {

ResumeProjectDialog::ResumeProjectDialog(const QString& lastProject,
                                           const QStringList& recentProjects,
                                           QWidget* parent)
    : QDialog(parent)
{
    setupUi(lastProject, recentProjects);
}

ResumeProjectDialog::~ResumeProjectDialog()
{
}

void ResumeProjectDialog::setupUi(const QString& lastProject, const QStringList& recentProjects)
{
    setWindowTitle(tr("Welcome to XXML Studio"));
    setMinimumWidth(500);
    setMinimumHeight(350);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    m_titleLabel = new QLabel(tr("Resume Previous Session?"), this);
    QFont titleFont = m_titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    mainLayout->addWidget(m_titleLabel);

    // Description
    m_descLabel = new QLabel(tr("Select a project to open, or start a new session:"), this);
    mainLayout->addWidget(m_descLabel);

    // Project list
    m_projectList = new QListWidget(this);
    m_projectList->setAlternatingRowColors(true);

    // Add last project at top if it exists
    QStringList allProjects;
    if (!lastProject.isEmpty() && QFileInfo::exists(lastProject)) {
        allProjects << lastProject;
    }

    // Add recent projects (avoiding duplicates)
    for (const QString& project : recentProjects) {
        if (!allProjects.contains(project) && QFileInfo::exists(project)) {
            allProjects << project;
        }
    }

    for (const QString& project : allProjects) {
        QFileInfo fi(project);
        QListWidgetItem* item = new QListWidgetItem(m_projectList);
        item->setText(fi.fileName() + "\n" + fi.absolutePath());
        item->setData(Qt::UserRole, project);

        // Mark the last project
        if (project == lastProject) {
            item->setText(fi.fileName() + " (Last Session)\n" + fi.absolutePath());
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
    }

    if (m_projectList->count() > 0) {
        m_projectList->setCurrentRow(0);
    }

    m_projectList->setStyleSheet(
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background-color: #0d47a1; }"
    );

    mainLayout->addWidget(m_projectList, 1);

    // Don't ask again checkbox
    m_dontAskCheckbox = new QCheckBox(tr("Don't ask again (can be changed in Settings)"), this);
    mainLayout->addWidget(m_dontAskCheckbox);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_newSessionButton = new QPushButton(tr("Start New Session"), this);
    buttonLayout->addWidget(m_newSessionButton);

    m_resumeButton = new QPushButton(tr("Open Selected"), this);
    m_resumeButton->setDefault(true);
    m_resumeButton->setEnabled(m_projectList->count() > 0);
    buttonLayout->addWidget(m_resumeButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_resumeButton, &QPushButton::clicked, this, &ResumeProjectDialog::onResumeClicked);
    connect(m_newSessionButton, &QPushButton::clicked, this, &ResumeProjectDialog::onNewSessionClicked);
    connect(m_projectList, &QListWidget::itemDoubleClicked, this, &ResumeProjectDialog::onProjectDoubleClicked);
    connect(m_projectList, &QListWidget::itemSelectionChanged, this, &ResumeProjectDialog::onSelectionChanged);
}

void ResumeProjectDialog::onResumeClicked()
{
    QListWidgetItem* item = m_projectList->currentItem();
    if (item) {
        m_selectedProject = item->data(Qt::UserRole).toString();
    }
    m_dontAskAgain = m_dontAskCheckbox->isChecked();
    accept();
}

void ResumeProjectDialog::onNewSessionClicked()
{
    m_selectedProject.clear();
    m_dontAskAgain = m_dontAskCheckbox->isChecked();
    accept();
}

void ResumeProjectDialog::onProjectDoubleClicked(QListWidgetItem* item)
{
    if (item) {
        m_selectedProject = item->data(Qt::UserRole).toString();
        m_dontAskAgain = m_dontAskCheckbox->isChecked();
        accept();
    }
}

void ResumeProjectDialog::onSelectionChanged()
{
    m_resumeButton->setEnabled(m_projectList->currentItem() != nullptr);
}

} // namespace XXMLStudio
