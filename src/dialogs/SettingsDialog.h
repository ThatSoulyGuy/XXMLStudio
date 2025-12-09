#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QFontComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

namespace XXMLStudio {

class Settings;

/**
 * Dialog for configuring IDE settings.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings* settings, QWidget* parent = nullptr);
    ~SettingsDialog();

public slots:
    void accept() override;

private slots:
    void browseToolchain();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    QWidget* createEditorTab();
    QWidget* createToolchainTab();
    QWidget* createAppearanceTab();

    Settings* m_settings = nullptr;
    QTabWidget* m_tabWidget = nullptr;

    // Editor settings
    QFontComboBox* m_fontCombo = nullptr;
    QSpinBox* m_fontSizeSpinBox = nullptr;
    QSpinBox* m_tabWidthSpinBox = nullptr;
    QCheckBox* m_showLineNumbersCheck = nullptr;
    QCheckBox* m_highlightCurrentLineCheck = nullptr;
    QCheckBox* m_wordWrapCheck = nullptr;

    // Toolchain settings
    QLineEdit* m_toolchainPathEdit = nullptr;

    // Appearance settings
    QComboBox* m_syntaxThemeCombo = nullptr;

    // Buttons
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QPushButton* m_applyButton = nullptr;
};

} // namespace XXMLStudio

#endif // SETTINGSDIALOG_H
