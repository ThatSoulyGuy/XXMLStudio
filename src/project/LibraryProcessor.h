#ifndef LIBRARYPROCESSOR_H
#define LIBRARYPROCESSOR_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "Project.h"

namespace XXMLStudio {

/**
 * Helper class for processing dependencies from cache to Library folder.
 *
 * Processing includes:
 * - Copying from cache to Library/{dep-name}/
 * - Removing .xxmlp project files
 * - Extracting DLLs to .dlls/ subfolder
 * - Pruning top-level folders without XXML source files
 */
class LibraryProcessor : public QObject
{
    Q_OBJECT

public:
    explicit LibraryProcessor(QObject* parent = nullptr);
    ~LibraryProcessor();

    /**
     * Process a cached dependency to the Library folder.
     *
     * @param cachePath Source path in global cache
     * @param libraryPath Destination path in project's Library folder
     * @param outDllFiles Output: list of DLL filenames moved to .dlls/
     * @return true if processing succeeded
     */
    bool processToLibrary(const QString& cachePath,
                         const QString& libraryPath,
                         QStringList& outDllFiles);

    /**
     * Extract transitive dependencies from a .xxmlp file.
     * Call this BEFORE processToLibrary() since it removes .xxmlp files.
     *
     * @param cachePath Path to cached dependency
     * @return List of dependencies found in the project file
     */
    QList<Dependency> extractTransitiveDependencies(const QString& cachePath);

    /**
     * Copy all DLLs from Library .dlls/ folders to the build output directory.
     *
     * @param libraryRoot Path to project's Library folder
     * @param outputDir Build output directory
     * @return Number of DLLs copied
     */
    int copyDllsToOutput(const QString& libraryRoot, const QString& outputDir);

signals:
    void progress(const QString& message);
    void error(const QString& message);

private:
    /**
     * Check if a directory tree contains any XXML files.
     */
    bool containsXxmlFiles(const QString& dir) const;

    /**
     * Find DLL files in the top-level of a directory (not recursive).
     */
    QStringList findTopLevelDlls(const QString& dir) const;

    /**
     * Remove top-level folders that don't contain XXML files.
     */
    void pruneNonSourceFolders(const QString& dir);

    /**
     * Remove .xxmlp project files from a directory (top-level only).
     */
    void removeProjectFiles(const QString& dir);

    /**
     * Copy a directory recursively.
     */
    bool copyDirectory(const QString& srcPath, const QString& dstPath);
};

} // namespace XXMLStudio

#endif // LIBRARYPROCESSOR_H
