#include "DependencyManager.h"
#include "GitFetcher.h"
#include "Project.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>

namespace XXMLStudio {

DependencyManager::DependencyManager(QObject* parent)
    : QObject(parent)
{
    m_fetcher = new GitFetcher(this);
    connect(m_fetcher, &GitFetcher::finished,
            this, &DependencyManager::onFetchFinished);
    connect(m_fetcher, &GitFetcher::error,
            this, &DependencyManager::onFetchError);
    connect(m_fetcher, &GitFetcher::progress,
            this, &DependencyManager::onFetchProgress);

    // Default cache directory
    QString appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    m_cacheDir = appData + "/XXMLStudio/dependencies";
    QDir().mkpath(m_cacheDir);
}

DependencyManager::~DependencyManager()
{
    cancel();
}

void DependencyManager::setCacheDir(const QString& path)
{
    m_cacheDir = path;
    QDir().mkpath(m_cacheDir);
}

void DependencyManager::resolveDependencies(Project* project)
{
    if (m_resolving) {
        emit error("Dependency resolution already in progress");
        return;
    }

    m_resolving = true;
    m_currentProject = project;
    m_resolvedPaths.clear();
    m_processedUrls.clear();
    m_pendingQueue.clear();

    emit resolutionStarted();

    // Queue all direct dependencies
    const QList<Dependency>& deps = project->dependencies();
    for (const Dependency& dep : deps) {
        PendingDependency pending;
        pending.name = dep.name;
        pending.gitUrl = dep.gitUrl;
        pending.tag = dep.tag;
        m_pendingQueue.enqueue(pending);
    }

    if (m_pendingQueue.isEmpty()) {
        m_resolving = false;
        emit resolutionFinished(true);
        return;
    }

    processNextDependency();
}

bool DependencyManager::isCached(const QString& gitUrl, const QString& tag) const
{
    QString path = getCachedPath(gitUrl, tag);
    return QDir(path).exists() && m_fetcher->isGitRepository(path);
}

QString DependencyManager::getCachedPath(const QString& gitUrl, const QString& tag) const
{
    QString urlPath = urlToPath(gitUrl);
    QString version = tag.isEmpty() ? "main" : tag;
    return m_cacheDir + "/" + urlPath + "/" + version;
}

void DependencyManager::clearCache()
{
    QDir cacheDir(m_cacheDir);
    cacheDir.removeRecursively();
    QDir().mkpath(m_cacheDir);
}

void DependencyManager::cancel()
{
    m_fetcher->cancel();
    m_resolving = false;
    m_pendingQueue.clear();
}

QStringList DependencyManager::getIncludePaths() const
{
    QStringList paths;
    for (auto it = m_resolvedPaths.constBegin(); it != m_resolvedPaths.constEnd(); ++it) {
        paths.append(it.value());
    }
    return paths;
}

void DependencyManager::processNextDependency()
{
    if (m_pendingQueue.isEmpty()) {
        m_resolving = false;
        emit resolutionFinished(true);
        return;
    }

    PendingDependency dep = m_pendingQueue.dequeue();

    // Check for cycles
    if (m_processedUrls.contains(dep.gitUrl)) {
        emit resolutionProgress(QString("Skipping already resolved: %1").arg(dep.name));
        processNextDependency();
        return;
    }

    m_processedUrls.append(dep.gitUrl);
    fetchDependency(dep);
}

void DependencyManager::fetchDependency(const PendingDependency& dep)
{
    QString cachedPath = getCachedPath(dep.gitUrl, dep.tag);

    if (isCached(dep.gitUrl, dep.tag)) {
        emit resolutionProgress(QString("Using cached: %1").arg(dep.name));
        m_resolvedPaths[dep.name] = cachedPath;
        emit dependencyResolved(dep.name, cachedPath);

        // Check for transitive dependencies
        parseTransitiveDependencies(cachedPath);

        processNextDependency();
    } else {
        emit resolutionProgress(QString("Fetching: %1").arg(dep.name));
        m_fetcher->clone(dep.gitUrl, cachedPath, dep.tag);
    }
}

QString DependencyManager::urlToPath(const QString& url) const
{
    // Convert URL like "https://github.com/user/repo" to "github.com/user/repo"
    QString path = url;

    // Remove protocol
    if (path.startsWith("https://")) {
        path = path.mid(8);
    } else if (path.startsWith("http://")) {
        path = path.mid(7);
    } else if (path.startsWith("git@")) {
        path = path.mid(4);
        path.replace(':', '/');
    }

    // Remove .git suffix
    if (path.endsWith(".git")) {
        path = path.left(path.length() - 4);
    }

    return path;
}

void DependencyManager::parseTransitiveDependencies(const QString& path)
{
    // Look for .xxmlp file in the dependency
    QDir depDir(path);
    QStringList projectFiles = depDir.entryList({"*.xxmlp"}, QDir::Files);

    if (projectFiles.isEmpty()) {
        return;
    }

    // Parse the first project file found
    QString projectPath = path + "/" + projectFiles.first();

    // Create a temporary Project to parse the file
    Project tempProject;
    if (!tempProject.load(projectPath)) {
        return;
    }

    // Queue transitive dependencies
    const QList<Dependency>& deps = tempProject.dependencies();
    for (const Dependency& dep : deps) {
        if (!m_processedUrls.contains(dep.gitUrl)) {
            PendingDependency pending;
            pending.name = dep.name;
            pending.gitUrl = dep.gitUrl;
            pending.tag = dep.tag;
            m_pendingQueue.enqueue(pending);
        }
    }
}

void DependencyManager::onFetchFinished(bool success, const QString& path)
{
    if (!m_resolving) {
        return;
    }

    if (success) {
        // Find which dependency was just fetched
        QString depName;
        for (auto it = m_processedUrls.constBegin(); it != m_processedUrls.constEnd(); ++it) {
            QString cachedPath = getCachedPath(*it, QString());
            if (cachedPath == path || path.startsWith(m_cacheDir)) {
                // Find name from original dependencies
                if (m_currentProject) {
                    for (const Dependency& dep : m_currentProject->dependencies()) {
                        QString checkPath = getCachedPath(dep.gitUrl, dep.tag);
                        if (checkPath == path) {
                            depName = dep.name;
                            break;
                        }
                    }
                }
                break;
            }
        }

        if (depName.isEmpty()) {
            // Extract from path
            depName = QFileInfo(path).fileName();
        }

        m_resolvedPaths[depName] = path;
        emit dependencyResolved(depName, path);

        // Check for transitive dependencies
        parseTransitiveDependencies(path);

        processNextDependency();
    } else {
        m_resolving = false;
        emit error(QString("Failed to fetch dependency"));
        emit resolutionFinished(false);
    }
}

void DependencyManager::onFetchError(const QString& message)
{
    emit error(message);

    if (m_resolving) {
        m_resolving = false;
        emit resolutionFinished(false);
    }
}

void DependencyManager::onFetchProgress(const QString& message)
{
    emit resolutionProgress(message);
}

} // namespace XXMLStudio
