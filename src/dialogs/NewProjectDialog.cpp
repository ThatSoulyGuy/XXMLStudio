#include "NewProjectDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>

namespace XXMLStudio {

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Project"));
    setMinimumWidth(500);
    setupUi();
}

NewProjectDialog::~NewProjectDialog()
{
}

void NewProjectDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title
    QLabel* titleLabel = new QLabel(tr("Create New XXML Project"), this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // Form
    QFormLayout* formLayout = new QFormLayout();

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("MyProject"));
    formLayout->addRow(tr("Project Name:"), m_nameEdit);

    // Location with browse button
    QHBoxLayout* locationLayout = new QHBoxLayout();
    m_locationEdit = new QLineEdit(this);
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    m_locationEdit->setText(defaultPath + "/XXMLProjects");
    locationLayout->addWidget(m_locationEdit);

    m_browseButton = new QPushButton(tr("Browse..."), this);
    locationLayout->addWidget(m_browseButton);
    formLayout->addRow(tr("Location:"), locationLayout);

    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Executable"), "executable");
    m_typeCombo->addItem(tr("Library"), "library");
    formLayout->addRow(tr("Project Type:"), m_typeCombo);

    mainLayout->addLayout(formLayout);

    // Spacer
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    m_createButton = new QPushButton(tr("Create"), this);
    m_createButton->setDefault(true);
    m_createButton->setEnabled(false);
    buttonLayout->addWidget(m_createButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_nameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::validateInput);
    connect(m_locationEdit, &QLineEdit::textChanged, this, &NewProjectDialog::validateInput);
    connect(m_browseButton, &QPushButton::clicked, this, &NewProjectDialog::browseLocation);
    connect(m_createButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString NewProjectDialog::projectName() const
{
    return m_nameEdit->text().trimmed();
}

QString NewProjectDialog::projectLocation() const
{
    return m_locationEdit->text().trimmed();
}

QString NewProjectDialog::projectType() const
{
    return m_typeCombo->currentData().toString();
}

void NewProjectDialog::browseLocation()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Project Location"),
        m_locationEdit->text());

    if (!dir.isEmpty()) {
        m_locationEdit->setText(dir);
    }
}

void NewProjectDialog::validateInput()
{
    bool valid = !m_nameEdit->text().trimmed().isEmpty() &&
                 !m_locationEdit->text().trimmed().isEmpty();
    m_createButton->setEnabled(valid);
}

} // namespace XXMLStudio
