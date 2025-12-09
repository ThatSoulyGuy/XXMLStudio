#include "DocumentSyncManager.h"
#include "LSPClient.h"

#include <QUrl>
#include <QFileInfo>

namespace XXMLStudio {

DocumentSyncManager::DocumentSyncManager(LSPClient* client, QObject* parent)
    : QObject(parent)
    , m_client(client)
{
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &DocumentSyncManager::flushPendingChanges);
}

DocumentSyncManager::~DocumentSyncManager()
{
    // Flush any pending changes before destruction
    flushPendingChanges();
}

void DocumentSyncManager::openDocument(const QString& filePath, const QString& text)
{
    QString uri = filePathToUri(filePath);

    DocumentState state;
    state.uri = uri;
    state.version = 1;
    state.hasPendingChanges = false;

    m_documents[filePath] = state;

    // Determine language ID from file extension
    QFileInfo fileInfo(filePath);
    QString languageId = "xxml";  // Default for XXML files

    QString ext = fileInfo.suffix().toLower();
    if (ext == "xxml") {
        languageId = "xxml";
    } else if (ext == "xxmlp") {
        languageId = "toml";  // Project files are TOML
    }

    m_client->openDocument(uri, languageId, state.version, text);
}

void DocumentSyncManager::closeDocument(const QString& filePath)
{
    if (!m_documents.contains(filePath)) {
        return;
    }

    QString uri = m_documents[filePath].uri;
    m_documents.remove(filePath);

    m_client->closeDocument(uri);
}

void DocumentSyncManager::documentChanged(const QString& filePath, const QString& text)
{
    if (!m_documents.contains(filePath)) {
        // Document not tracked, open it first
        openDocument(filePath, text);
        return;
    }

    DocumentState& state = m_documents[filePath];
    state.pendingText = text;
    state.hasPendingChanges = true;

    // Restart debounce timer
    m_debounceTimer->start(m_debounceDelay);
}

void DocumentSyncManager::documentSaved(const QString& filePath, const QString& text)
{
    if (!m_documents.contains(filePath)) {
        return;
    }

    // Flush any pending changes first
    if (m_documents[filePath].hasPendingChanges) {
        DocumentState& state = m_documents[filePath];
        state.version++;
        m_client->changeDocument(state.uri, state.version, state.pendingText);
        state.hasPendingChanges = false;
    }

    QString uri = m_documents[filePath].uri;
    m_client->saveDocument(uri, text);
}

int DocumentSyncManager::documentVersion(const QString& filePath) const
{
    if (m_documents.contains(filePath)) {
        return m_documents[filePath].version;
    }
    return 0;
}

void DocumentSyncManager::flushPendingChanges()
{
    for (auto it = m_documents.begin(); it != m_documents.end(); ++it) {
        DocumentState& state = it.value();

        if (state.hasPendingChanges) {
            state.version++;
            m_client->changeDocument(state.uri, state.version, state.pendingText);
            state.hasPendingChanges = false;
            state.pendingText.clear();
        }
    }
}

QString DocumentSyncManager::filePathToUri(const QString& path)
{
    return QUrl::fromLocalFile(path).toString();
}

QString DocumentSyncManager::uriToFilePath(const QString& uri)
{
    return QUrl(uri).toLocalFile();
}

} // namespace XXMLStudio
