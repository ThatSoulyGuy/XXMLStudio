#include "SettingsDialog.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>

namespace XXMLStudio {

SettingsDialog::SettingsDialog(Settings* settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(500, 400);
    setupUi();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Tab widget
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createEditorTab(), tr("Editor"));
    m_tabWidget->addTab(createToolchainTab(), tr("Toolchain"));
    m_tabWidget->addTab(createAppearanceTab(), tr("Appearance"));
    mainLayout->addWidget(m_tabWidget);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    m_applyButton = new QPushButton(tr("Apply"), this);
    buttonLayout->addWidget(m_applyButton);

    m_okButton = new QPushButton(tr("OK"), this);
    m_okButton->setDefault(true);
    buttonLayout->addWidget(m_okButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
}

QWidget* SettingsDialog::createEditorTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Font group
    QGroupBox* fontGroup = new QGroupBox(tr("Font"), widget);
    QFormLayout* fontLayout = new QFormLayout(fontGroup);

    m_fontCombo = new QFontComboBox(widget);
    m_fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    fontLayout->addRow(tr("Font family:"), m_fontCombo);

    m_fontSizeSpinBox = new QSpinBox(widget);
    m_fontSizeSpinBox->setRange(6, 72);
    m_fontSizeSpinBox->setValue(12);
    fontLayout->addRow(tr("Font size:"), m_fontSizeSpinBox);

    layout->addWidget(fontGroup);

    // Display group
    QGroupBox* displayGroup = new QGroupBox(tr("Display"), widget);
    QVBoxLayout* displayLayout = new QVBoxLayout(displayGroup);

    m_showLineNumbersCheck = new QCheckBox(tr("Show line numbers"), widget);
    m_showLineNumbersCheck->setChecked(true);
    displayLayout->addWidget(m_showLineNumbersCheck);

    m_highlightCurrentLineCheck = new QCheckBox(tr("Highlight current line"), widget);
    m_highlightCurrentLineCheck->setChecked(true);
    displayLayout->addWidget(m_highlightCurrentLineCheck);

    m_wordWrapCheck = new QCheckBox(tr("Word wrap"), widget);
    displayLayout->addWidget(m_wordWrapCheck);

    layout->addWidget(displayGroup);

    // Indentation group
    QGroupBox* indentGroup = new QGroupBox(tr("Indentation"), widget);
    QFormLayout* indentLayout = new QFormLayout(indentGroup);

    m_tabWidthSpinBox = new QSpinBox(widget);
    m_tabWidthSpinBox->setRange(1, 16);
    m_tabWidthSpinBox->setValue(4);
    indentLayout->addRow(tr("Tab width:"), m_tabWidthSpinBox);

    layout->addWidget(indentGroup);

    layout->addStretch();
    return widget;
}

QWidget* SettingsDialog::createToolchainTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QGroupBox* group = new QGroupBox(tr("XXML Toolchain"), widget);
    QVBoxLayout* groupLayout = new QVBoxLayout(group);

    QLabel* infoLabel = new QLabel(
        tr("Specify the path to the XXML toolchain directory.\n"
           "Leave empty to use the bundled toolchain."),
        widget);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #888;");
    groupLayout->addWidget(infoLabel);

    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_toolchainPathEdit = new QLineEdit(widget);
    m_toolchainPathEdit->setPlaceholderText(tr("Use bundled toolchain"));
    pathLayout->addWidget(m_toolchainPathEdit);

    QPushButton* browseButton = new QPushButton(tr("Browse..."), widget);
    connect(browseButton, &QPushButton::clicked, this, &SettingsDialog::browseToolchain);
    pathLayout->addWidget(browseButton);

    groupLayout->addLayout(pathLayout);
    layout->addWidget(group);

    layout->addStretch();
    return widget;
}

QWidget* SettingsDialog::createAppearanceTab()
{
    QWidget* widget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    // Syntax highlighting theme group
    QGroupBox* syntaxGroup = new QGroupBox(tr("Syntax Highlighting"), widget);
    QFormLayout* syntaxLayout = new QFormLayout(syntaxGroup);

    m_syntaxThemeCombo = new QComboBox(widget);
    m_syntaxThemeCombo->addItem(tr("IntelliJ Darcula"), 0);
    m_syntaxThemeCombo->addItem(tr("Qt Creator Dark"), 1);
    m_syntaxThemeCombo->addItem(tr("VS Code Dark+"), 2);
    syntaxLayout->addRow(tr("Color theme:"), m_syntaxThemeCombo);

    QLabel* themeInfo = new QLabel(
        tr("Choose a color scheme for syntax highlighting.\n"
           "Changes take effect immediately."),
        widget);
    themeInfo->setStyleSheet("color: #888;");
    themeInfo->setWordWrap(true);
    syntaxLayout->addRow(themeInfo);

    layout->addWidget(syntaxGroup);

    layout->addStretch();
    return widget;
}

void SettingsDialog::loadSettings()
{
    if (!m_settings) return;

    m_fontCombo->setCurrentFont(m_settings->editorFont());
    m_fontSizeSpinBox->setValue(m_settings->editorFontSize());
    m_tabWidthSpinBox->setValue(m_settings->tabWidth());
    m_showLineNumbersCheck->setChecked(m_settings->showLineNumbers());
    m_highlightCurrentLineCheck->setChecked(m_settings->highlightCurrentLine());
    m_wordWrapCheck->setChecked(m_settings->wordWrap());
    m_toolchainPathEdit->setText(m_settings->toolchainPath());

    // Appearance
    int themeIndex = m_syntaxThemeCombo->findData(m_settings->syntaxTheme());
    if (themeIndex >= 0) {
        m_syntaxThemeCombo->setCurrentIndex(themeIndex);
    }
}

void SettingsDialog::saveSettings()
{
    if (!m_settings) return;

    m_settings->setEditorFont(m_fontCombo->currentFont());
    m_settings->setEditorFontSize(m_fontSizeSpinBox->value());
    m_settings->setTabWidth(m_tabWidthSpinBox->value());
    m_settings->setShowLineNumbers(m_showLineNumbersCheck->isChecked());
    m_settings->setHighlightCurrentLine(m_highlightCurrentLineCheck->isChecked());
    m_settings->setWordWrap(m_wordWrapCheck->isChecked());
    m_settings->setToolchainPath(m_toolchainPathEdit->text());

    // Appearance
    m_settings->setSyntaxTheme(m_syntaxThemeCombo->currentData().toInt());

    m_settings->sync();
}

void SettingsDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void SettingsDialog::browseToolchain()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select Toolchain Directory"),
        m_toolchainPathEdit->text());

    if (!dir.isEmpty()) {
        m_toolchainPathEdit->setText(dir);
    }
}

} // namespace XXMLStudio
