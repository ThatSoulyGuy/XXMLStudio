#include "LSPClient.h"
#include "JsonRpcClient.h"

#include <QDebug>
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

    m_serverPath = serverPath;
    setState(State::Connecting);

    // Build command-line arguments with -I for include paths
    QStringList args;
    for (const QString& path : m_includePaths) {
        args << "-I" << path;
    }

    if (!m_rpc->start(serverPath, args)) {
        setState(State::Disconnected);
        return false;
    }

    return true;
}

void LSPClient::restart()
{
    if (m_serverPath.isEmpty()) {
        return;
    }

    // Clear pending requests
    m_pendingCompletions.clear();
    m_pendingHovers.clear();
    m_pendingDefinitions.clear();
    m_pendingReferences.clear();
    m_pendingSymbols.clear();

    // If already disconnected, start immediately
    if (m_state == State::Disconnected) {
        start(m_serverPath);
        return;
    }

    // Otherwise, set pending restart flag and stop
    // onServerStopped will trigger the restart
    m_pendingRestart = true;
    stop();
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

    // Check if we need to restart
    if (m_pendingRestart) {
        m_pendingRestart = false;
        if (!m_serverPath.isEmpty()) {
            start(m_serverPath);
        }
    }
}

void LSPClient::onServerError(const QString& errorMsg)
{
    emit error(errorMsg);
}

void LSPClient::setProjectRoot(const QString& path)
{
    m_rootPath = path;
}

void LSPClient::setIncludePaths(const QStringList& paths)
{
    m_includePaths = paths;
}

void LSPClient::updateConfiguration()
{
    if (!isReady()) return;

    // Send workspace/didChangeConfiguration notification
    QJsonObject settings;
    QJsonArray includePathsArray;
    for (const QString& path : m_includePaths) {
        includePathsArray.append(path);
    }
    settings["includePaths"] = includePathsArray;

    QJsonObject params;
    params["settings"] = settings;

    m_rpc->sendNotification("workspace/didChangeConfiguration", params);
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

    // Pass include paths as initialization options
    if (!m_includePaths.isEmpty()) {
        QJsonObject initOptions;
        QJsonArray includePathsArray;
        for (const QString& path : m_includePaths) {
            includePathsArray.append(path);
        }
        initOptions["includePaths"] = includePathsArray;
        params["initializationOptions"] = initOptions;
    }

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
        [this](const QJsonValue& result, const QJsonObject& err) {
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
    if (!isReady()) {
        qDebug() << "LSPClient::requestCompletion - not ready";
        return;
    }

    qDebug() << "LSPClient::requestCompletion" << uri << "line" << line << "char" << character;

    QJsonObject params;
    params["textDocument"] = QJsonObject{{"uri", uri}};
    params["position"] = QJsonObject{{"line", line}, {"character", character}};

    int id = m_rpc->sendRequest("textDocument/completion", params,
        [this, uri](const QJsonValue& result, const QJsonObject& err) {
            qDebug() << "LSPClient: completion response received, isArray:" << result.isArray()
                     << "isObject:" << result.isObject() << "isNull:" << result.isNull();

            if (!err.isEmpty()) {
                qDebug() << "LSPClient: completion error:" << err["message"].toString();
                emit this->error(QString("Completion failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPCompletionItem> items;
            QJsonArray itemsArray;

            // Handle both CompletionList (object with items) and CompletionItem[] (direct array)
            if (result.isArray()) {
                itemsArray = result.toArray();
                qDebug() << "LSPClient: got array with" << itemsArray.size() << "items";
            } else if (result.isObject()) {
                QJsonObject obj = result.toObject();
                if (obj.contains("items")) {
                    itemsArray = obj["items"].toArray();
                    qDebug() << "LSPClient: got object with" << itemsArray.size() << "items";
                }
            }

            for (const auto& item : itemsArray) {
                items.append(LSPCompletionItem::fromJson(item.toObject()));
            }

            qDebug() << "LSPClient: emitting completionReceived with" << items.size() << "items";
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
        [this, uri](const QJsonValue& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("Hover failed: %1").arg(err["message"].toString()));
                return;
            }

            if (result.isNull() || !result.isObject()) {
                emit hoverReceived(uri, LSPHover{});
                return;
            }

            LSPHover hover = LSPHover::fromJson(result.toObject());
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
        [this, uri](const QJsonValue& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("Definition failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPLocation> locations;
            // Can be null, a single Location, or an array of Locations
            if (result.isObject()) {
                QJsonObject obj = result.toObject();
                if (obj.contains("uri")) {
                    locations.append(LSPLocation::fromJson(obj));
                }
            } else if (result.isArray()) {
                for (const auto& loc : result.toArray()) {
                    locations.append(LSPLocation::fromJson(loc.toObject()));
                }
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
        [this, uri](const QJsonValue& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("References failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPLocation> locations;
            if (result.isArray()) {
                for (const auto& loc : result.toArray()) {
                    locations.append(LSPLocation::fromJson(loc.toObject()));
                }
            }
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
        [this, uri](const QJsonValue& result, const QJsonObject& err) {
            if (!err.isEmpty()) {
                emit this->error(QString("DocumentSymbol failed: %1").arg(err["message"].toString()));
                return;
            }

            QList<LSPDocumentSymbol> symbols;
            if (result.isArray()) {
                for (const auto& sym : result.toArray()) {
                    symbols.append(LSPDocumentSymbol::fromJson(sym.toObject()));
                }
            }
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
