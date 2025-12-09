#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

namespace XXMLStudio {

/**
 * Represents a bookmark in a file.
 */
struct Bookmark {
    QString filePath;
    int line;           // 1-based line number
    QString lineText;   // Optional: text of the line for display
};

/**
 * Manages bookmarks across all open files.
 * Bookmarks persist per-file and allow quick navigation.
 */
class BookmarkManager : public QObject
{
    Q_OBJECT

public:
    explicit BookmarkManager(QObject* parent = nullptr);
    ~BookmarkManager();

    // Bookmark operations
    void toggleBookmark(const QString& filePath, int line, const QString& lineText = QString());
    void addBookmark(const QString& filePath, int line, const QString& lineText = QString());
    void removeBookmark(const QString& filePath, int line);
    void removeAllBookmarks(const QString& filePath);
    void clearAllBookmarks();

    // Query
    bool hasBookmark(const QString& filePath, int line) const;
    QList<int> bookmarksForFile(const QString& filePath) const;
    QList<Bookmark> allBookmarks() const;
    int bookmarkCount() const;

    // Navigation
    Bookmark nextBookmark(const QString& currentFile, int currentLine) const;
    Bookmark previousBookmark(const QString& currentFile, int currentLine) const;
    Bookmark nextBookmarkInFile(const QString& filePath, int currentLine) const;
    Bookmark previousBookmarkInFile(const QString& filePath, int currentLine) const;

    // Line adjustment (when lines are inserted/deleted)
    void adjustBookmarks(const QString& filePath, int fromLine, int delta);

signals:
    void bookmarkAdded(const QString& filePath, int line);
    void bookmarkRemoved(const QString& filePath, int line);
    void bookmarksCleared(const QString& filePath);
    void allBookmarksCleared();
    void bookmarksChanged();

private:
    // Map from file path to list of bookmarked lines (sorted)
    QMap<QString, QList<Bookmark>> m_bookmarks;

    void sortBookmarks(const QString& filePath);
};

} // namespace XXMLStudio

#endif // BOOKMARKMANAGER_H
