#include "BookmarkManager.h"

#include <algorithm>

namespace XXMLStudio {

BookmarkManager::BookmarkManager(QObject* parent)
    : QObject(parent)
{
}

BookmarkManager::~BookmarkManager()
{
}

void BookmarkManager::toggleBookmark(const QString& filePath, int line, const QString& lineText)
{
    if (hasBookmark(filePath, line)) {
        removeBookmark(filePath, line);
    } else {
        addBookmark(filePath, line, lineText);
    }
}

void BookmarkManager::addBookmark(const QString& filePath, int line, const QString& lineText)
{
    if (hasBookmark(filePath, line)) {
        return;
    }

    Bookmark bm;
    bm.filePath = filePath;
    bm.line = line;
    bm.lineText = lineText;

    m_bookmarks[filePath].append(bm);
    sortBookmarks(filePath);

    emit bookmarkAdded(filePath, line);
    emit bookmarksChanged();
}

void BookmarkManager::removeBookmark(const QString& filePath, int line)
{
    if (!m_bookmarks.contains(filePath)) {
        return;
    }

    auto& list = m_bookmarks[filePath];
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].line == line) {
            list.removeAt(i);
            if (list.isEmpty()) {
                m_bookmarks.remove(filePath);
            }
            emit bookmarkRemoved(filePath, line);
            emit bookmarksChanged();
            return;
        }
    }
}

void BookmarkManager::removeAllBookmarks(const QString& filePath)
{
    if (m_bookmarks.remove(filePath) > 0) {
        emit bookmarksCleared(filePath);
        emit bookmarksChanged();
    }
}

void BookmarkManager::clearAllBookmarks()
{
    m_bookmarks.clear();
    emit allBookmarksCleared();
    emit bookmarksChanged();
}

bool BookmarkManager::hasBookmark(const QString& filePath, int line) const
{
    if (!m_bookmarks.contains(filePath)) {
        return false;
    }

    const auto& list = m_bookmarks[filePath];
    for (const auto& bm : list) {
        if (bm.line == line) {
            return true;
        }
    }
    return false;
}

QList<int> BookmarkManager::bookmarksForFile(const QString& filePath) const
{
    QList<int> lines;
    if (m_bookmarks.contains(filePath)) {
        for (const auto& bm : m_bookmarks[filePath]) {
            lines.append(bm.line);
        }
    }
    return lines;
}

QList<Bookmark> BookmarkManager::allBookmarks() const
{
    QList<Bookmark> all;
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        all.append(it.value());
    }
    return all;
}

int BookmarkManager::bookmarkCount() const
{
    int count = 0;
    for (auto it = m_bookmarks.constBegin(); it != m_bookmarks.constEnd(); ++it) {
        count += it.value().size();
    }
    return count;
}

Bookmark BookmarkManager::nextBookmark(const QString& currentFile, int currentLine) const
{
    QList<Bookmark> all = allBookmarks();
    if (all.isEmpty()) {
        return Bookmark();
    }

    // Sort all bookmarks by file then line
    std::sort(all.begin(), all.end(), [](const Bookmark& a, const Bookmark& b) {
        if (a.filePath != b.filePath) {
            return a.filePath < b.filePath;
        }
        return a.line < b.line;
    });

    // Find current position
    int currentIdx = -1;
    for (int i = 0; i < all.size(); ++i) {
        if (all[i].filePath == currentFile && all[i].line > currentLine) {
            return all[i];
        }
        if (all[i].filePath == currentFile && all[i].line == currentLine) {
            currentIdx = i;
        }
        if (all[i].filePath > currentFile && currentIdx == -1) {
            return all[i];
        }
    }

    // Wrap around to first bookmark
    return all[0];
}

Bookmark BookmarkManager::previousBookmark(const QString& currentFile, int currentLine) const
{
    QList<Bookmark> all = allBookmarks();
    if (all.isEmpty()) {
        return Bookmark();
    }

    // Sort all bookmarks by file then line
    std::sort(all.begin(), all.end(), [](const Bookmark& a, const Bookmark& b) {
        if (a.filePath != b.filePath) {
            return a.filePath < b.filePath;
        }
        return a.line < b.line;
    });

    // Find previous bookmark
    Bookmark prev;
    for (int i = all.size() - 1; i >= 0; --i) {
        if (all[i].filePath == currentFile && all[i].line < currentLine) {
            return all[i];
        }
        if (all[i].filePath < currentFile) {
            return all[i];
        }
    }

    // Wrap around to last bookmark
    return all.last();
}

Bookmark BookmarkManager::nextBookmarkInFile(const QString& filePath, int currentLine) const
{
    if (!m_bookmarks.contains(filePath)) {
        return Bookmark();
    }

    const auto& list = m_bookmarks[filePath];
    for (const auto& bm : list) {
        if (bm.line > currentLine) {
            return bm;
        }
    }

    // Wrap around to first bookmark in file
    if (!list.isEmpty()) {
        return list.first();
    }

    return Bookmark();
}

Bookmark BookmarkManager::previousBookmarkInFile(const QString& filePath, int currentLine) const
{
    if (!m_bookmarks.contains(filePath)) {
        return Bookmark();
    }

    const auto& list = m_bookmarks[filePath];
    for (int i = list.size() - 1; i >= 0; --i) {
        if (list[i].line < currentLine) {
            return list[i];
        }
    }

    // Wrap around to last bookmark in file
    if (!list.isEmpty()) {
        return list.last();
    }

    return Bookmark();
}

void BookmarkManager::adjustBookmarks(const QString& filePath, int fromLine, int delta)
{
    if (!m_bookmarks.contains(filePath) || delta == 0) {
        return;
    }

    auto& list = m_bookmarks[filePath];
    bool changed = false;

    for (int i = list.size() - 1; i >= 0; --i) {
        if (list[i].line >= fromLine) {
            int newLine = list[i].line + delta;
            if (newLine < 1) {
                // Bookmark deleted
                list.removeAt(i);
            } else {
                list[i].line = newLine;
            }
            changed = true;
        }
    }

    if (changed) {
        if (list.isEmpty()) {
            m_bookmarks.remove(filePath);
        }
        emit bookmarksChanged();
    }
}

void BookmarkManager::sortBookmarks(const QString& filePath)
{
    if (!m_bookmarks.contains(filePath)) {
        return;
    }

    auto& list = m_bookmarks[filePath];
    std::sort(list.begin(), list.end(), [](const Bookmark& a, const Bookmark& b) {
        return a.line < b.line;
    });
}

} // namespace XXMLStudio
