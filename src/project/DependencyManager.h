#ifndef DEPENDENCYMANAGER_H
#define DEPENDENCYMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QQueue>

namespace XXMLStudio {

class Project;
class GitFetcher;
struct Dependency;

/**
 * Manages project dependencies.
 * Handles fetching, caching, and resolving dependencies.
 */
class DependencyManager : public QObject
{
    Q_OBJECT

public:
    explicit DependencyManager(QObject* parent = nullptr);
    ~DependencyManager();

    // Set the cache directory for dependencies
    void setCacheDir(const QString& path);
    QString cacheDir() const { return m_cacheDir; }

    // Resolve all dependencies for a project
    void resolveDependencies(Project* project);

    // Check if a specific dependency is cached
    bool isCached(const QString& gitUrl, const QString& tag) const;

    // Get the cached path for a dependency
    QString getCachedPath(const QString& gitUrl, const QString& tag) const;

    // Clear the dependency cache
    void clearCache();

    // Cancel ongoing operations
    void cancel();

    // Check if resolution is in progress
    bool isResolving() const { return m_resolving; }

    // Get resolved include paths for all dependencies
    QStringList getIncludePaths() const;

signals:
    void resolutionStarted();
    void resolutionProgress(const QString& message);
    void dependencyResolved(const QString& name, const QString& path);
    void resolutionFinished(bool success);
    void error(const QString& message);

private slots:
    void onFetchFinished(bool success, const QString& path);
    void onFetchError(const QString& message);
    void onFetchProgress(const QString& message);

private:
    struct PendingDependency {
        QString name;
        QString gitUrl;
        QString tag;
    };

    void processNextDependency();
    void fetchDependency(const PendingDependency& dep);
    QString urlToPath(const QString& url) const;
    void parseTransitiveDependencies(const QString& path);

    GitFetcher* m_fetcher = nullptr;
    QString m_cacheDir;
    bool m_resolving = false;
    QQueue<PendingDependency> m_pendingQueue;
    QMap<QString, QString> m_resolvedPaths; // name -> local path
    QStringList m_processedUrls; // Track already processed URLs to avoid cycles
    Project* m_currentProject = nullptr;
};

} // namespace XXMLStudio

#endif // DEPENDENCYMANAGER_H
