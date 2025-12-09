#include "LSPClient.h"
#include "JsonRpcClient.h"

#include <QJsonArray>
#include <QUrl>
#include <QDir>
#include <QCoreApplication>

namespace XXMLStudio {

LSPClient::LSPClient(QObject* parent)
    : QObject(parent)
{
    m_rpc = new JsonRpcClient(this);

    connect(m_rpc, &JsonRpcClient::serverStarted,
            this, &LSPClient::onServerStarted);
    connect(m_rpc, &JsonRpcClient::serverStopped,
            this, &LSPClient::onServerStopped);
    connect(m_rpc, &JsonRpcClient::serverError,
            this, &LSPClient::onServerError);
    connect(m_rpc, &JsonRpcClient::notificationReceived,
            this, &LSPClient::onNotificationReceived);
    connect(m_rpc, &JsonRpcClient::logMessage,
            this, &LSPClient::logMessage);
}

LSPClient::~LSPClient()
{
    stop();
}

bool LSPClient::start(const QString& serverPath)
{
    if (m_state != State::Disconnected) {
        return false;
    }

    setState(State::Connecting);

    if (!m_rpc->start(serverPath)) {
        setState(State::Disconnected);
        return false;
    }

    return true;
}

void LSPClient::stop()
{
    if (m_state == State::Disconnected) {
        return;
    }

    setState(State::ShuttingDown);
    m_rpc->stop();
}

void LSPClient::onServerStarted()
{
    initialize();
}

void LSPClient::onServerStopped()
{
    setState(State::Disconnected);
}

void LSPClient::onServerError(const QString& errorMsg)
{
    emit error(errorMsg);
}

void LSPClient::setProjectRoot(const QString& path)
{
    m_rootPath = path;
}

void LSPClient::initialize()
{
    setState(State::Initializing);

    // Use project root if set, otherwise fall back to current path
    QString rootPath = m_rootPath.isEmpty() ? QDir::currentPath() : m_rootPath;

    QJsonObject params;
    params["processId"] = static_cast<int>(QCoreApplication::applicationPid());
    params["rootPath"] = rootPath;
    params["rootUri"] = filePathToUri(rootPath);

    QJsonObject capabilities;
    QJsonObject textDocumentCapabilities;

    textDocumentCapabilities["synchronization"] = QJsonObject{
        {"dynamicRegistration", false},
        {"willSave", false},
        {"willSaveWaitUntil", false},
        {"didSave", true}
    };

    textDocumentCapabilities["completion"] = QJsonObject{
        {"dynamicRegistration", false},
        {"completionItem", QJsonObject{{"snippetSupport", false}}}
    };

    textDocumentCapabilities["hover"] = QJsonObject{{"dynamicRegistration", false}};
    textDocumentCapabilities["definition"] = QJsonObject{{"dynamicRegistration", false}};
    textDocumentCapabilities["references"] = QJsonObject{{"dynamicRegistration", false}};
    textDocumentCapabilities["documentSymbol"] = QJsonObject{
        {"dynamicRegistration", false},
        {"hierarchicalDocumentSymbolSupport", true}
    };
    textDocumentCapabilities["publishDiagnostics"] = QJsonObject{{"relatedInformation", true}};

    capabilities["textDocument"] = textDocumentCapabilities;
    params["capabilities"] = capabilities;

    m_rpc->sendRequest("initialize", params,
        [this](const QJsonObject& result, const QJsonObject& err) {
            Q_UNUSED(result)
            if (!err.isEmpty()) {
                emit this->error(QString("Initialize failed: %1").arg(err["message"].toString()));
                setState(State::Disconnected);
                return;
            }

            m_rpc->sendNotification("initialized", QJsonObject{});
            setState(State::Ready);
            emit initialized();
        });
}

void LSPClient::setState(State state)
{
    if (m_state != state) {
        m_state = state;
        emit stateChanged(state);
    }
}

void LSPClient::openDocument(const QString& uri, const QString& languageId, int version, const QString& text)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{
        {"uri", uri},
        {"languageId", languageId},
        {"version", version},
        {"text", text}
    };

    m_rpc->sendNotification("textDocument/didOpen", params);
}

void LSPClient::closeDocument(const QString& uri)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    m_rpc->sendNotification("textDocument/didClose", params);
}

void LSPClient::changeDocument(const QString& uri, int version, const QString& text)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}, {"version", version}};
    params["contentChanges"] = QJsonArray{QJsonObject{{"text", text}}};

    m_rpc->sendNotification("textDocument/didChange", params);
}

void LSPClient::saveDocument(const QString& uri, const QString& text)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["text"] = text;

    m_rpc->sendNotification("textDocument/didSave", params);
}

void LSPClient::requestCompletion(const QString& uri, int line, int character)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    int id = m_rpc->sendRequest("textDocument/completion", params,
        [this, uri](const QJsonObject& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("Completion failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPCompletionItem> items;
            QJsonArray itemsArray;

            if (result.contains("items")) {
                itemsArray = result["items"].toArray();
            }

            for (const auto& item : itemsArray) {
                items.append(LSPCompletionItem::fromJson(item.toObject()));
            }

            emit completionReceived(uri, items);
        });

    m_pendingCompletions[id] = uri;
}

void LSPClient::requestHover(const QString& uri, int line, int character)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    int id = m_rpc->sendRequest("textDocument/hover", params,
        [this, uri](const QJsonObject& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("Hover failed: %1").arg(err["message"].toString()));
                return;
            }

            if (result.isEmpty()) {
                emit hoverReceived(uri, LSPHover{});
                return;
            }

            LSPHover hover = LSPHover::fromJson(result);
            emit hoverReceived(uri, hover);
        });

    m_pendingHovers[id] = uri;
}

void LSPClient::requestDefinition(const QString& uri, int line, int character)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    int id = m_rpc->sendRequest("textDocument/definition", params,
        [this, uri](const QJsonObject& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("Definition failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPLocation> locations;
            if (result.contains("uri")) {
                locations.append(LSPLocation::fromJson(result));
            }

            emit definitionReceived(uri, locations);
        });

    m_pendingDefinitions[id] = uri;
}

void LSPClient::requestReferences(const QString& uri, int line, int character)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};
    params["context"] = QJsonObject{{"includeDeclaration", true}};

    int id = m_rpc->sendRequest("textDocument/references", params,
        [this, uri](const QJsonObject& result, const QJsonObject& err) {
            Q_UNUSED(result)
            if (!err.isEmpty()) {
                emit this->error(QString("References failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPLocation> locations;
            emit referencesReceived(uri, locations);
        });

    m_pendingReferences[id] = uri;
}

void LSPClient::requestDocumentSymbols(const QString& uri)
{
    if (!isReady()) return;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};

    int id = m_rpc->sendRequest("textDocument/documentSymbol", params,
        [this, uri](const QJsonObject& result, const QJsonObject& err) {
            Q_UNUSED(result)
            if (!err.isEmpty()) {
                emit this->error(QString("DocumentSymbol failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPDocumentSymbol> symbols;
            emit documentSymbolsReceived(uri, symbols);
        });

    m_pendingSymbols[id] = uri;
}

void LSPClient::onNotificationReceived(const QString& method, const QJsonObject& params)
{
    if (method == "textDocument/publishDiagnostics") {
        QString uri = params["uri"].toString();
        QJsonArray diagnosticsArray = params["diagnostics"].toArray();

        QList<LSPDiagnostic> diagnostics;
        for (const auto& diag : diagnosticsArray) {
            diagnostics.append(LSPDiagnostic::fromJson(diag.toObject()));
        }

        emit diagnosticsReceived(uri, diagnostics);
    }
    else if (method == "window/logMessage") {
        QString message = params["message"].toString();
        emit logMessage(QString("Server: %1").arg(message));
    }
}

QString LSPClient::filePathToUri(const QString& path) const
{
    return QUrl::fromLocalFile(path).toString();
}

QString LSPClient::uriToFilePath(const QString& uri) const
{
    return QUrl(uri).toLocalFile();
}

} // namespace XXMLStudio
