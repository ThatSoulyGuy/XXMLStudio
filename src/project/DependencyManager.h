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
class LibraryProcessor;
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

    // Set the project root for Library folder
    void setProjectRoot(const QString& path);
    QString projectRoot() const { return m_projectRoot; }

    // Get the Library path for a dependency
    QString getLibraryPath(const QString& depName) const;

    // Copy all dependency DLLs to the build output directory
    void copyDllsToOutput(const QString& outputDir);

    // Resolve all dependencies for a project
    void resolveDependencies(Project* project);

    // Check if a specific dependency is cached
    bool isCached(const QString& gitUrl, const QString& tag) const;

    // Get the cached path for a dependency
    QString getCachedPath(const QString& gitUrl, const QString& tag) const;

    // Clear the dependency cache
    void clearCache();
    void clearCacheFor(const QString& gitUrl, const QString& tag);

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
    bool processToLibraryFolder(const QString& depName, const QString& cachePath);

    GitFetcher* m_fetcher = nullptr;
    LibraryProcessor* m_processor = nullptr;
    QString m_cacheDir;
    QString m_projectRoot;
    bool m_resolving = false;
    QQueue<PendingDependency> m_pendingQueue;
    QMap<QString, QString> m_resolvedPaths; // name -> Library path
    QMap<QString, QStringList> m_dependencyDlls; // name -> DLL filenames
    QStringList m_processedUrls; // Track already processed URLs to avoid cycles
    Project* m_currentProject = nullptr;
    PendingDependency m_currentDependency; // Currently being fetched
};

} // namespace XXMLStudio

#endif // DEPENDENCYMANAGER_H
