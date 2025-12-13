#include "AddDependencyDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDir>
#include <QUrl>
#include <QRegularExpression>

#include "project/GitFetcher.h"

namespace XXMLStudio {

AddDependencyDialog::AddDependencyDialog(const QString& cacheDir, QWidget* parent)
    : QDialog(parent)
    , m_cacheDir(cacheDir)
{
    setWindowTitle(tr("Add Dependency"));
    setMinimumWidth(550);
    setupUi();

    m_fetcher = new GitFetcher(this);
    connect(m_fetcher, &GitFetcher::finished, this, &AddDependencyDialog::onFetchFinished);
    connect(m_fetcher, &GitFetcher::error, this, &AddDependencyDialog::onFetchError);
    connect(m_fetcher, &GitFetcher::progress, this, &AddDependencyDialog::onFetchProgress);
}

AddDependencyDialog::~AddDependencyDialog()
{
}

void AddDependencyDialog::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Info label
    QLabel* infoLabel = new QLabel(
        tr("Enter a Git repository URL. The repository must contain an XXML "
           "library project (no entry point)."),
        this);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #888; margin-bottom: 10px;");
    mainLayout->addWidget(infoLabel);

    // Form
    QFormLayout* formLayout = new QFormLayout();

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setPlaceholderText(tr("github.com/user/repo or https://github.com/user/repo"));
    formLayout->addRow(tr("Repository URL:"), m_urlEdit);

    m_tagEdit = new QLineEdit(this);
    m_tagEdit->setPlaceholderText(tr("v1.0.0, main, or leave empty for default branch"));
    formLayout->addRow(tr("Tag/Branch:"), m_tagEdit);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Auto-generated from repository name"));
    formLayout->addRow(tr("Name (alias):"), m_nameEdit);

    mainLayout->addLayout(formLayout);

    // Status section
    mainLayout->addSpacing(10);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // Indeterminate
    m_progressBar->setVisible(false);
    m_progressBar->setTextVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Spacer
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancel"), this);
    buttonLayout->addWidget(m_cancelButton);

    m_validateButton = new QPushButton(tr("Validate && Fetch"), this);
    m_validateButton->setEnabled(false);
    buttonLayout->addWidget(m_validateButton);

    m_addButton = new QPushButton(tr("Add"), this);
    m_addButton->setDefault(true);
    m_addButton->setEnabled(false);
    buttonLayout->addWidget(m_addButton);

    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_urlEdit, &QLineEdit::textChanged, this, &AddDependencyDialog::validateInput);
    connect(m_validateButton, &QPushButton::clicked, this, &AddDependencyDialog::onValidateClicked);
    connect(m_addButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddDependencyDialog::validateInput()
{
    QString url = m_urlEdit->text().trimmed();
    bool valid = !url.isEmpty();
    m_validateButton->setEnabled(valid && !m_isValidating);

    // Reset validation state if URL changes
    if (m_isValidated) {
        m_isValidated = false;
        m_addButton->setEnabled(false);
        m_statusLabel->clear();
    }
}

void AddDependencyDialog::onValidateClicked()
{
    if (m_isValidating) {
        // Cancel operation
        m_fetcher->cancel();
        setValidating(false);
        m_statusLabel->setText(tr("Validation cancelled."));
        m_statusLabel->setStyleSheet("color: #888;");
        return;
    }

    QString url = m_urlEdit->text().trimmed();
    QString tag = m_tagEdit->text().trimmed();

    // Normalize URL - add https:// if not present
    if (!url.startsWith("https://") && !url.startsWith("http://") && !url.startsWith("git@")) {
        url = "https://" + url;
    }

    // Calculate target path in cache
    QString targetPath = m_cacheDir + "/" + urlToPath(url) + "/" + (tag.isEmpty() ? "default" : tag);

    setValidating(true);
    m_hasError = false;  // Reset error flag
    m_statusLabel->setText(tr("Fetching repository..."));
    m_statusLabel->setStyleSheet("color: #888;");

    m_fetcher->clone(url, targetPath, tag);
}

void AddDependencyDialog::onFetchFinished(bool success, const QString& path)
{
    setValidating(false);

    if (!success) {
        // Only show generic message if no specific error was shown by onFetchError
        if (!m_hasError) {
            m_statusLabel->setText(tr("Failed to fetch repository. Check the URL and try again."));
            m_statusLabel->setStyleSheet("color: #ff5555;");
        }
        return;
    }

    // Validate that it's a library project
    if (!validateProjectType(path)) {
        // Error message is set in validateProjectType
        return;
    }

    // Success - populate validated dependency
    QString url = m_urlEdit->text().trimmed();
    if (!url.startsWith("https://") && !url.startsWith("http://") && !url.startsWith("git@")) {
        url = "https://" + url;
    }

    m_validatedDep.gitUrl = url;
    m_validatedDep.tag = m_tagEdit->text().trimmed();
    m_validatedDep.cachePath = path;  // Store cache path, localPath will be set during resolution
    m_validatedDep.localPath.clear(); // Will be set to Library/{name}/ during resolution
    m_validatedDep.commitHash = m_fetcher->getCurrentCommit(path);

    // Extract repo name from URL
    QString repoName = url;
    if (repoName.endsWith(".git")) {
        repoName.chop(4);
    }
    repoName = repoName.section('/', -1);

    // Determine final name (user alias or repo name)
    QString userAlias = m_nameEdit->text().trimmed();
    QString name;

    if (repoName.contains('-')) {
        // Repo name has dashes - require alias without dashes
        if (userAlias.isEmpty()) {
            m_statusLabel->setText(
                tr("Error: Repository name '%1' contains dashes. "
                   "Please provide an alias without dashes in the Name field.")
                .arg(repoName));
            m_statusLabel->setStyleSheet("color: #ff5555;");
            return;
        }
        if (userAlias.contains('-')) {
            m_statusLabel->setText(
                tr("Error: Alias '%1' cannot contain dashes. "
                   "Please use underscores or camelCase instead.")
                .arg(userAlias));
            m_statusLabel->setStyleSheet("color: #ff5555;");
            return;
        }
        name = userAlias;
    } else {
        // No dashes in repo name - use alias if provided, otherwise use repo name
        if (!userAlias.isEmpty()) {
            if (userAlias.contains('-')) {
                m_statusLabel->setText(
                    tr("Error: Alias '%1' cannot contain dashes. "
                       "Please use underscores or camelCase instead.")
                    .arg(userAlias));
                m_statusLabel->setStyleSheet("color: #ff5555;");
                return;
            }
            name = userAlias;
        } else {
            name = repoName;
        }
    }

    m_validatedDep.name = name;

    m_statusLabel->setText(tr("Validated successfully: %1 (library project)").arg(name));
    m_statusLabel->setStyleSheet("color: #50fa7b;");

    m_isValidated = true;
    m_addButton->setEnabled(true);
}

void AddDependencyDialog::onFetchError(const QString& message)
{
    setValidating(false);
    m_hasError = true;  // Mark that we showed a specific error
    m_statusLabel->setText(tr("Error: %1").arg(message));
    m_statusLabel->setStyleSheet("color: #ff5555;");
}

void AddDependencyDialog::onFetchProgress(const QString& message)
{
    // Show progress messages (truncate if too long)
    QString displayMsg = message;
    if (displayMsg.length() > 80) {
        displayMsg = displayMsg.left(77) + "...";
    }
    m_statusLabel->setText(displayMsg);
}

void AddDependencyDialog::setValidating(bool validating)
{
    m_isValidating = validating;
    m_progressBar->setVisible(validating);
    m_urlEdit->setEnabled(!validating);
    m_tagEdit->setEnabled(!validating);
    m_nameEdit->setEnabled(!validating);
    m_addButton->setEnabled(false);

    if (validating) {
        m_validateButton->setText(tr("Cancel"));
    } else {
        m_validateButton->setText(tr("Validate && Fetch"));
        m_validateButton->setEnabled(!m_urlEdit->text().trimmed().isEmpty());
    }
}

bool AddDependencyDialog::validateProjectType(const QString& path)
{
    // Find .xxmlp file in the directory
    QDir dir(path);
    QStringList projectFiles = dir.entryList({"*.xxmlp"}, QDir::Files);

    if (projectFiles.isEmpty()) {
        m_statusLabel->setText(tr("Error: No .xxmlp project file found in the repository."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return false;
    }

    // Load and check project type
    Project tempProject;
    QString projectPath = path + "/" + projectFiles.first();

    if (!tempProject.load(projectPath)) {
        m_statusLabel->setText(tr("Error: Could not parse the project file."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return false;
    }

    // Must be a Library type (no entry point)
    if (tempProject.type() != Project::Type::Library) {
        m_statusLabel->setText(
            tr("Error: This is not a library project. Dependencies must be "
               "library projects without an entry point."));
        m_statusLabel->setStyleSheet("color: #ff5555;");
        return false;
    }

    return true;
}

QString AddDependencyDialog::urlToPath(const QString& url) const
{
    QString path = url;

    // Remove protocol
    path.remove(QRegularExpression("^https?://"));
    path.remove(QRegularExpression("^git@"));

    // Replace : with / for git@ style URLs
    path.replace(':', '/');

    // Remove .git suffix
    if (path.endsWith(".git")) {
        path.chop(4);
    }

    return path;
}

} // namespace XXMLStudio
