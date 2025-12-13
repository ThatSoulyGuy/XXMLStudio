#include "SetUpstreamDialog.h"

#include <QFormLayout>
#include <QGroupBox>

namespace XXMLStudio {

SetUpstreamDialog::SetUpstreamDialog(const QString& currentBranch,
                                     const QStringList& existingRemotes,
                                     QWidget* parent)
    : QDialog(parent)
    , m_currentBranch(currentBranch)
    , m_existingRemotes(existingRemotes)
{
    setWindowTitle(tr("Set Upstream Branch"));
    setMinimumWidth(450);
    setupUi();
}

SetUpstreamDialog::~SetUpstreamDialog()
{
}

void SetUpstreamDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Info label
    m_infoLabel = new QLabel(this);
    m_infoLabel->setWordWrap(true);
    if (m_existingRemotes.isEmpty()) {
        m_infoLabel->setText(tr("The branch '%1' has no upstream branch and no remotes are configured.\n\n"
                                "Please add a remote to push your changes.")
                             .arg(m_currentBranch));
    } else {
        m_infoLabel->setText(tr("The branch '%1' has no upstream branch.\n\n"
                                "Select an existing remote or add a new one to push your changes.")
                             .arg(m_currentBranch));
    }
    mainLayout->addWidget(m_infoLabel);

    // Remote selection
    QGroupBox* remoteGroup = new QGroupBox(tr("Remote Configuration"), this);
    QVBoxLayout* remoteLayout = new QVBoxLayout(remoteGroup);

    // Remote combo box
    QFormLayout* comboForm = new QFormLayout();
    m_remoteCombo = new QComboBox(this);

    // Add existing remotes
    for (const QString& remote : m_existingRemotes) {
        m_remoteCombo->addItem(remote);
    }
    // Add option to create new remote
    m_remoteCombo->addItem(tr("-- Add new remote --"));

    comboForm->addRow(tr("Remote:"), m_remoteCombo);
    remoteLayout->addLayout(comboForm);

    // New remote inputs (initially hidden if remotes exist)
    QFormLayout* newRemoteForm = new QFormLayout();

    m_remoteNameLabel = new QLabel(tr("Name:"), this);
    m_remoteNameEdit = new QLineEdit(this);
    m_remoteNameEdit->setPlaceholderText(tr("e.g., origin"));
    m_remoteNameEdit->setText("origin");
    newRemoteForm->addRow(m_remoteNameLabel, m_remoteNameEdit);

    m_remoteUrlLabel = new QLabel(tr("URL:"), this);
    m_remoteUrlEdit = new QLineEdit(this);
    m_remoteUrlEdit->setPlaceholderText(tr("e.g., https://github.com/user/repo.git"));
    newRemoteForm->addRow(m_remoteUrlLabel, m_remoteUrlEdit);

    remoteLayout->addLayout(newRemoteForm);
    mainLayout->addWidget(remoteGroup);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_okButton = new QPushButton(tr("Push"), this);
    m_okButton->setDefault(true);
    m_okButton->setEnabled(false);
    buttonLayout->addWidget(m_okButton);

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_remoteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SetUpstreamDialog::onRemoteSelectionChanged);
    connect(m_remoteNameEdit, &QLineEdit::textChanged,
            this, &SetUpstreamDialog::validateInput);
    connect(m_remoteUrlEdit, &QLineEdit::textChanged,
            this, &SetUpstreamDialog::validateInput);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Initial state
    if (m_existingRemotes.isEmpty()) {
        // No remotes - show new remote inputs, hide combo
        m_remoteCombo->setVisible(false);
        m_remoteNameLabel->setVisible(true);
        m_remoteNameEdit->setVisible(true);
        m_remoteUrlLabel->setVisible(true);
        m_remoteUrlEdit->setVisible(true);
        m_remoteUrlEdit->setFocus();
    } else {
        // Have remotes - select first one, hide new remote inputs
        m_remoteCombo->setCurrentIndex(0);
        onRemoteSelectionChanged(0);
    }

    validateInput();
}

void SetUpstreamDialog::onRemoteSelectionChanged(int index)
{
    bool isNewRemote = (index == m_remoteCombo->count() - 1);

    m_remoteNameLabel->setVisible(isNewRemote);
    m_remoteNameEdit->setVisible(isNewRemote);
    m_remoteUrlLabel->setVisible(isNewRemote);
    m_remoteUrlEdit->setVisible(isNewRemote);

    if (isNewRemote) {
        m_remoteUrlEdit->setFocus();
    }

    validateInput();
}

void SetUpstreamDialog::validateInput()
{
    bool valid = false;

    if (isNewRemote()) {
        // New remote - need both name and URL
        valid = !m_remoteNameEdit->text().trimmed().isEmpty() &&
                !m_remoteUrlEdit->text().trimmed().isEmpty();
    } else if (!m_existingRemotes.isEmpty()) {
        // Existing remote selected
        valid = true;
    }

    m_okButton->setEnabled(valid);
}

QString SetUpstreamDialog::remoteName() const
{
    if (isNewRemote() || m_existingRemotes.isEmpty()) {
        return m_remoteNameEdit->text().trimmed();
    }
    return m_remoteCombo->currentText();
}

QString SetUpstreamDialog::remoteUrl() const
{
    return m_remoteUrlEdit->text().trimmed();
}

bool SetUpstreamDialog::isNewRemote() const
{
    if (m_existingRemotes.isEmpty()) {
        return true;
    }
    return m_remoteCombo->currentIndex() == m_remoteCombo->count() - 1;
}

} // namespace XXMLStudio
