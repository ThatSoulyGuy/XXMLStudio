#include "FindReplaceDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>

namespace XXMLStudio {

FindReplaceDialog::FindReplaceDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Find and Replace"));
    setMinimumWidth(450);
    setupUi();
}

FindReplaceDialog::~FindReplaceDialog()
{
}

void FindReplaceDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Search and replace fields
    QFormLayout* formLayout = new QFormLayout();

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search text..."));
    formLayout->addRow(tr("Find:"), m_searchEdit);

    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("Replace with..."));
    formLayout->addRow(tr("Replace:"), m_replaceEdit);

    mainLayout->addLayout(formLayout);

    // Options
    QHBoxLayout* optionsLayout = new QHBoxLayout();

    m_caseSensitiveCheck = new QCheckBox(tr("Case sensitive"), this);
    optionsLayout->addWidget(m_caseSensitiveCheck);

    m_wholeWordCheck = new QCheckBox(tr("Whole word"), this);
    optionsLayout->addWidget(m_wholeWordCheck);

    m_regexCheck = new QCheckBox(tr("Regular expression"), this);
    optionsLayout->addWidget(m_regexCheck);

    optionsLayout->addStretch();
    mainLayout->addLayout(optionsLayout);

    // Status label
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QGridLayout* buttonLayout = new QGridLayout();

    m_findPrevButton = new QPushButton(tr("Find Previous"), this);
    buttonLayout->addWidget(m_findPrevButton, 0, 0);

    m_findNextButton = new QPushButton(tr("Find Next"), this);
    m_findNextButton->setDefault(true);
    buttonLayout->addWidget(m_findNextButton, 0, 1);

    m_replaceButton = new QPushButton(tr("Replace"), this);
    buttonLayout->addWidget(m_replaceButton, 1, 0);

    m_replaceAllButton = new QPushButton(tr("Replace All"), this);
    buttonLayout->addWidget(m_replaceAllButton, 1, 1);

    m_closeButton = new QPushButton(tr("Close"), this);
    buttonLayout->addWidget(m_closeButton, 2, 1);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_findNextButton, &QPushButton::clicked, this, &FindReplaceDialog::findNext);
    connect(m_findPrevButton, &QPushButton::clicked, this, &FindReplaceDialog::findPrevious);
    connect(m_replaceButton, &QPushButton::clicked, this, &FindReplaceDialog::replace);
    connect(m_replaceAllButton, &QPushButton::clicked, this, &FindReplaceDialog::replaceAll);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::close);

    // Also search on Enter in the search field
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &FindReplaceDialog::findNext);
}

QString FindReplaceDialog::searchText() const
{
    return m_searchEdit->text();
}

QString FindReplaceDialog::replaceText() const
{
    return m_replaceEdit->text();
}

bool FindReplaceDialog::caseSensitive() const
{
    return m_caseSensitiveCheck->isChecked();
}

bool FindReplaceDialog::wholeWord() const
{
    return m_wholeWordCheck->isChecked();
}

bool FindReplaceDialog::useRegex() const
{
    return m_regexCheck->isChecked();
}

void FindReplaceDialog::setSearchText(const QString& text)
{
    m_searchEdit->setText(text);
    m_searchEdit->selectAll();
}

} // namespace XXMLStudio
