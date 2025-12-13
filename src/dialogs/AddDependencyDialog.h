#ifndef ADDDEPENDENCYDIALOG_H
#define ADDDEPENDENCYDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include "project/Project.h"

namespace XXMLStudio {

class GitFetcher;

/**
 * Dialog for adding a new dependency with validation.
 * Fetches the dependency and verifies it's a library project (no entry point).
 */
class AddDependencyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddDependencyDialog(const QString& cacheDir, QWidget* parent = nullptr);
    ~AddDependencyDialog();

    // Get the validated dependency data
    Dependency dependency() const { return m_validatedDep; }

private slots:
    void onValidateClicked();
    void onFetchFinished(bool success, const QString& path);
    void onFetchError(const QString& message);
    void onFetchProgress(const QString& message);
    void validateInput();

private:
    void setupUi();
    void setValidating(bool validating);
    bool validateProjectType(const QString& path);
    QString urlToPath(const QString& url) const;

    // UI Elements
    QLineEdit* m_urlEdit = nullptr;
    QLineEdit* m_tagEdit = nullptr;
    QLineEdit* m_nameEdit = nullptr;
    QPushButton* m_validateButton = nullptr;
    QPushButton* m_addButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;

    // State
    GitFetcher* m_fetcher = nullptr;
    QString m_cacheDir;
    Dependency m_validatedDep;
    bool m_isValidating = false;
    bool m_isValidated = false;
    bool m_hasError = false;  // Track if error was already shown
};

} // namespace XXMLStudio

#endif // ADDDEPENDENCYDIALOG_H
