#include "GoToLineDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

namespace XXMLStudio {

GoToLineDialog::GoToLineDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Go to Line"));
    setFixedSize(300, 120);
    setupUi();
}

GoToLineDialog::~GoToLineDialog()
{
}

void GoToLineDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Form
    QFormLayout* formLayout = new QFormLayout();

    m_lineSpinBox = new QSpinBox(this);
    m_lineSpinBox->setMinimum(1);
    m_lineSpinBox->setMaximum(999999);
    m_lineSpinBox->selectAll();
    formLayout->addRow(tr("Line number:"), m_lineSpinBox);

    mainLayout->addLayout(formLayout);

    // Info label
    m_infoLabel = new QLabel(this);
    m_infoLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_infoLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    m_goButton = new QPushButton(tr("Go"), this);
    m_goButton->setDefault(true);
    buttonLayout->addWidget(m_goButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_goButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_lineSpinBox, &QSpinBox::editingFinished, this, &QDialog::accept);
}

void GoToLineDialog::setMaxLine(int max)
{
    m_lineSpinBox->setMaximum(max);
    m_infoLabel->setText(tr("(1 - %1)").arg(max));
}

void GoToLineDialog::setCurrentLine(int line)
{
    m_lineSpinBox->setValue(line);
    m_lineSpinBox->selectAll();
}

int GoToLineDialog::selectedLine() const
{
    return m_lineSpinBox->value();
}

} // namespace XXMLStudio
