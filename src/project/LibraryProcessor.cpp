#include "LibraryProcessor.h"
#include "ProjectFileParser.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

namespace XXMLStudio {

LibraryProcessor::LibraryProcessor(QObject* parent)
    : QObject(parent)
{
}

LibraryProcessor::~LibraryProcessor()
{
}

bool LibraryProcessor::processToLibrary(const QString& cachePath,
                                        const QString& libraryPath,
                                        QStringList& outDllFiles)
{
    outDllFiles.clear();

    // Validate source exists
    QDir cacheDir(cachePath);
    if (!cacheDir.exists()) {
        emit error(QString("Cache path does not exist: %1").arg(cachePath));
        return false;
    }

    // Clean existing library folder if it exists
    QDir libDir(libraryPath);
    if (libDir.exists()) {
        emit progress(QString("Cleaning existing: %1").arg(libraryPath));
        if (!libDir.removeRecursively()) {
            emit error(QString("Failed to clean existing library folder: %1").arg(libraryPath));
            return false;
        }
    }

    // Copy cache to library folder
    emit progress(QString("Copying to Library: %1").arg(libraryPath));
    if (!copyDirectory(cachePath, libraryPath)) {
        emit error(QString("Failed to copy to library folder: %1").arg(libraryPath));
        return false;
    }

    // Remove .xxmlp project files from library copy
    emit progress("Removing project files...");
    removeProjectFiles(libraryPath);

    // Find ALL DLLs recursively in the library copy (before pruning removes them)
    QStringList dllPaths = findTopLevelDlls(libraryPath);

    // Move DLLs to .dlls/ subfolder
    if (!dllPaths.isEmpty()) {
        QString dllsDir = libraryPath + "/.dlls";
        QDir().mkpath(dllsDir);

        emit progress(QString("Moving %1 DLL(s) to .dlls/").arg(dllPaths.size()));

        for (const QString& srcPath : dllPaths) {
            QString dllName = QFileInfo(srcPath).fileName();
            QString dstPath = dllsDir + "/" + dllName;

            // Move (rename) the file
            if (QFile::exists(srcPath)) {
                // Remove destination if exists (in case of duplicates)
                if (QFile::exists(dstPath)) {
                    QFile::remove(dstPath);
                }
                if (QFile::rename(srcPath, dstPath)) {
                    outDllFiles.append(dllName);
                    emit progress(QString("  Found DLL: %1").arg(dllName));
                } else {
                    // Try copy + delete if rename fails (cross-device)
                    if (QFile::copy(srcPath, dstPath)) {
                        QFile::remove(srcPath);
                        outDllFiles.append(dllName);
                        emit progress(QString("  Found DLL: %1").arg(dllName));
                    } else {
                        emit error(QString("Failed to move DLL: %1").arg(dllName));
                    }
                }
            }
        }
    }

    // Prune top-level folders without XXML files
    emit progress("Pruning non-source folders...");
    pruneNonSourceFolders(libraryPath);

    emit progress("Library processing complete.");
    return true;
}

QList<Dependency> LibraryProcessor::extractTransitiveDependencies(const QString& cachePath)
{
    QList<Dependency> dependencies;

    // Find .xxmlp file in cache root
    QDir cacheDir(cachePath);
    QStringList projectFiles = cacheDir.entryList({"*.xxmlp"}, QDir::Files);

    if (projectFiles.isEmpty()) {
        return dependencies;
    }

    QString projectFilePath = cachePath + "/" + projectFiles.first();

    // Parse the project file
    ProjectFileParser parser;
    if (!parser.parse(projectFilePath)) {
        emit error(QString("Failed to parse project file: %1").arg(projectFilePath));
        return dependencies;
    }

    // Extract dependencies
    if (parser.hasSection("Dependencies")) {
        auto section = parser.section("Dependencies");
        for (auto it = section.values.constBegin(); it != section.values.constEnd(); ++it) {
            Dependency dep;
            dep.name = it.key();

            // Parse "github.com/user/repo@v1.0.0" format
            QString value = it.value();
            int atPos = value.indexOf('@');
            if (atPos != -1) {
                dep.gitUrl = "https://" + value.left(atPos);
                dep.tag = value.mid(atPos + 1);
            } else {
                dep.gitUrl = "https://" + value;
            }
            dependencies.append(dep);
        }
    }

    return dependencies;
}

int LibraryProcessor::copyDllsToOutput(const QString& libraryRoot, const QString& outputDir)
{
    int count = 0;
    QDir libRootDir(libraryRoot);

    if (!libRootDir.exists()) {
        return 0;
    }

    // Ensure output directory exists
    QDir().mkpath(outputDir);

    // Iterate over each dependency folder in Library
    QStringList depFolders = libRootDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& depFolder : depFolders) {
        QString dllsPath = libraryRoot + "/" + depFolder + "/.dlls";
        QDir dllsDir(dllsPath);

        if (!dllsDir.exists()) {
            continue;
        }

        // Copy each DLL to output
        QStringList dlls = dllsDir.entryList({"*.dll", "*.DLL"}, QDir::Files);
        for (const QString& dllName : dlls) {
            QString srcPath = dllsPath + "/" + dllName;
            QString dstPath = outputDir + "/" + dllName;

            // Remove existing if present
            if (QFile::exists(dstPath)) {
                QFile::remove(dstPath);
            }

            if (QFile::copy(srcPath, dstPath)) {
                emit progress(QString("Copied DLL: %1").arg(dllName));
                count++;
            } else {
                emit error(QString("Failed to copy DLL: %1").arg(dllName));
            }
        }
    }

    return count;
}

bool LibraryProcessor::containsXxmlFiles(const QString& dir) const
{
    QDirIterator it(dir, {"*.XXML", "*.xxml"}, QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext();
}

QStringList LibraryProcessor::findTopLevelDlls(const QString& dir) const
{
    // Find ALL DLLs recursively (not just top-level)
    // This catches DLLs in bin/, lib/, etc. subfolders
    QStringList dllPaths;
    QDirIterator it(dir, {"*.dll", "*.DLL"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        dllPaths << it.next();
    }
    return dllPaths;
}

void LibraryProcessor::pruneNonSourceFolders(const QString& dir)
{
    QDir directory(dir);
    QStringList subDirs = directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString& subDir : subDirs) {
        // Skip .dlls folder
        if (subDir == ".dlls") {
            continue;
        }

        // Skip .git folder
        if (subDir == ".git") {
            // Actually remove .git folder - it's not needed in Library
            QString gitPath = dir + "/" + subDir;
            QDir gitDir(gitPath);
            gitDir.removeRecursively();
            continue;
        }

        QString subPath = dir + "/" + subDir;

        // Check if this folder or its children contain XXML files
        if (!containsXxmlFiles(subPath)) {
            emit progress(QString("Pruning folder (no XXML files): %1").arg(subDir));
            QDir subDirObj(subPath);
            subDirObj.removeRecursively();
        }
    }
}

void LibraryProcessor::removeProjectFiles(const QString& dir)
{
    QDir directory(dir);
    QStringList projectFiles = directory.entryList({"*.xxmlp"}, QDir::Files);

    for (const QString& file : projectFiles) {
        QString filePath = dir + "/" + file;
        if (QFile::remove(filePath)) {
            emit progress(QString("Removed project file: %1").arg(file));
        }
    }
}

bool LibraryProcessor::copyDirectory(const QString& srcPath, const QString& dstPath)
{
    QDir srcDir(srcPath);
    if (!srcDir.exists()) {
        return false;
    }

    // Create destination directory
    QDir dstDir(dstPath);
    if (!dstDir.exists()) {
        if (!QDir().mkpath(dstPath)) {
            return false;
        }
    }

    // Copy files
    QStringList files = srcDir.entryList(QDir::Files);
    for (const QString& file : files) {
        QString srcFilePath = srcPath + "/" + file;
        QString dstFilePath = dstPath + "/" + file;

        if (!QFile::copy(srcFilePath, dstFilePath)) {
            return false;
        }
    }

    // Copy directories recursively
    QStringList dirs = srcDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& dir : dirs) {
        QString srcSubPath = srcPath + "/" + dir;
        QString dstSubPath = dstPath + "/" + dir;

        if (!copyDirectory(srcSubPath, dstSubPath)) {
            return false;
        }
    }

    return true;
}

} // namespace XXMLStudio
