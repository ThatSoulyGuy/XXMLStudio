#ifndef LSPCLIENT_H
#define LSPCLIENT_H

#include <QObject>
#include <QString>
#include <QMap>
#include "LSPProtocol.h"

namespace XXMLStudio {

class JsonRpcClient;

/**
 * High-level LSP client that provides async API for LSP operations.
 */
class LSPClient : public QObject
{
    Q_OBJECT

public:
    enum class State {
        Disconnected,
        Connecting,
        Initializing,
        Ready,
        ShuttingDown
    };

    explicit LSPClient(QObject* parent = nullptr);
    ~LSPClient();

    // Connection management
    bool start(const QString& serverPath);
    void stop();
    State state() const { return m_state; }
    bool isReady() const { return m_state == State::Ready; }

    // Project root for LSP
    void setProjectRoot(const QString& path);
    QString projectRoot() const { return m_rootPath; }

    // Document management
    void openDocument(const QString& uri, const QString& languageId, int version, const QString& text);
    void closeDocument(const QString& uri);
    void changeDocument(const QString& uri, int version, const QString& text);
    void saveDocument(const QString& uri, const QString& text);

    // Language features
    void requestCompletion(const QString& uri, int line, int character);
    void requestHover(const QString& uri, int line, int character);
    void requestDefinition(const QString& uri, int line, int character);
    void requestReferences(const QString& uri, int line, int character);
    void requestDocumentSymbols(const QString& uri);

signals:
    void stateChanged(State state);
    void initialized();
    void error(const QString& message);
    void logMessage(const QString& message);

    // Diagnostics
    void diagnosticsReceived(const QString& uri, const QList<LSPDiagnostic>& diagnostics);

    // Completion
    void completionReceived(const QString& uri, const QList<LSPCompletionItem>& items);

    // Hover
    void hoverReceived(const QString& uri, const LSPHover& hover);

    // Definition/References
    void definitionReceived(const QString& uri, const QList<LSPLocation>& locations);
    void referencesReceived(const QString& uri, const QList<LSPLocation>& locations);

    // Document symbols
    void documentSymbolsReceived(const QString& uri, const QList<LSPDocumentSymbol>& symbols);

private slots:
    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString& error);
    void onNotificationReceived(const QString& method, const QJsonObject& params);

private:
    void initialize();
    void setState(State state);
    QString filePathToUri(const QString& path) const;
    QString uriToFilePath(const QString& uri) const;

    JsonRpcClient* m_rpc = nullptr;
    State m_state = State::Disconnected;
    QString m_rootPath;

    // Track pending request URIs for routing responses
    QMap<int, QString> m_pendingCompletions;
    QMap<int, QString> m_pendingHovers;
    QMap<int, QString> m_pendingDefinitions;
    QMap<int, QString> m_pendingReferences;
    QMap<int, QString> m_pendingSymbols;
};

} // namespace XXMLStudio

#endif // LSPCLIENT_H
