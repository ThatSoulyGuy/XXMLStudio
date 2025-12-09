#ifndef DOCUMENTSYNCMANAGER_H
#define DOCUMENTSYNCMANAGER_H

#include <QObject>
#include <QMap>
#include <QTimer>

namespace XXMLStudio {

class LSPClient;

/**
 * Manages document synchronization with the LSP server.
 * Debounces document changes to avoid overwhelming the server.
 */
class DocumentSyncManager : public QObject
{
    Q_OBJECT

public:
    explicit DocumentSyncManager(LSPClient* client, QObject* parent = nullptr);
    ~DocumentSyncManager();

    // Set debounce delay in milliseconds (default 300ms)
    void setDebounceDelay(int ms) { m_debounceDelay = ms; }
    int debounceDelay() const { return m_debounceDelay; }

    // Document lifecycle
    void openDocument(const QString& filePath, const QString& text);
    void closeDocument(const QString& filePath);
    void documentChanged(const QString& filePath, const QString& text);
    void documentSaved(const QString& filePath, const QString& text);

    // Get document version
    int documentVersion(const QString& filePath) const;

    // Convert between file path and URI
    static QString filePathToUri(const QString& path);
    static QString uriToFilePath(const QString& uri);

private slots:
    void flushPendingChanges();

private:
    struct DocumentState {
        QString uri;
        QString pendingText;
        int version = 0;
        bool hasPendingChanges = false;
    };

    LSPClient* m_client;
    QMap<QString, DocumentState> m_documents;  // filePath -> state
    QTimer* m_debounceTimer;
    int m_debounceDelay = 300;
};

} // namespace XXMLStudio

#endif // DOCUMENTSYNCMANAGER_H
