#include "JsonRpcClient.h"

#include <QJsonDocument>
#include <QRegularExpression>

namespace XXMLStudio {

JsonRpcClient::JsonRpcClient(QObject* parent)
    : QObject(parent)
{
    m_process = new QProcess(this);

    // Use separate channels to properly handle both stdout and stderr
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &JsonRpcClient::onReadyReadStandardOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &JsonRpcClient::onReadyReadStandardError);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &JsonRpcClient::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &JsonRpcClient::onProcessError);
}

JsonRpcClient::~JsonRpcClient()
{
    stop();
}

bool JsonRpcClient::start(const QString& serverPath, const QStringList& arguments)
{
    if (isRunning()) {
        return true;
    }

    m_process->start(serverPath, arguments);
    if (!m_process->waitForStarted(5000)) {
        emit serverError(tr("Failed to start LSP server: %1").arg(m_process->errorString()));
        return false;
    }

    emit serverStarted();
    emit logMessage(tr("LSP server started: %1").arg(serverPath));
    return true;
}

void JsonRpcClient::stop()
{
    if (!isRunning()) {
        return;
    }

    // Send shutdown request
    sendRequest("shutdown", QJsonObject{}, [this](const QJsonValue&, const QJsonObject&) {
        // Send exit notification
        sendNotification("exit", QJsonObject{});

        // Give server a chance to exit gracefully
        if (!m_process->waitForFinished(3000)) {
            m_process->terminate();
            if (!m_process->waitForFinished(2000)) {
                m_process->kill();
            }
        }
    });
}

bool JsonRpcClient::isRunning() const
{
    return m_process && m_process->state() == QProcess::Running;
}

int JsonRpcClient::sendRequest(const QString& method, const QJsonObject& params, ResponseCallback callback)
{
    int id = m_nextRequestId++;

    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = id;
    request["method"] = method;
    if (!params.isEmpty()) {
        request["params"] = params;
    }

    m_pendingRequests[id] = callback;
    writeMessage(request);

    emit logMessage(tr("Request [%1]: %2").arg(id).arg(method));
    return id;
}

void JsonRpcClient::sendNotification(const QString& method, const QJsonObject& params)
{
    QJsonObject notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    if (!params.isEmpty()) {
        notification["params"] = params;
    }

    writeMessage(notification);
    emit logMessage(tr("Notification: %1").arg(method));
}

void JsonRpcClient::onReadyReadStandardOutput()
{
    m_readBuffer.append(m_process->readAllStandardOutput());
    processIncomingData();
}

void JsonRpcClient::onReadyReadStandardError()
{
    QString errorOutput = QString::fromUtf8(m_process->readAllStandardError());
    emit logMessage(tr("LSP Server: %1").arg(errorOutput.trimmed()));
}

void JsonRpcClient::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    emit serverStopped();
    emit logMessage(tr("LSP server stopped"));
}

void JsonRpcClient::onProcessError(QProcess::ProcessError error)
{
    Q_UNUSED(error)
    emit serverError(m_process->errorString());
}

void JsonRpcClient::processIncomingData()
{
    // Parse Content-Length header and JSON body
    while (!m_readBuffer.isEmpty()) {
        // Look for Content-Length header
        static QRegularExpression headerRegex(R"(Content-Length:\s*(\d+)\r\n\r\n)");
        QString bufferStr = QString::fromUtf8(m_readBuffer);

        QRegularExpressionMatch match = headerRegex.match(bufferStr);
        if (!match.hasMatch()) {
            // Not enough data yet, or no valid header
            break;
        }

        int headerEnd = match.capturedEnd();
        int contentLength = match.captured(1).toInt();

        // Check if we have enough data for the body
        if (m_readBuffer.size() < headerEnd + contentLength) {
            break;  // Wait for more data
        }

        // Extract the JSON body
        QByteArray jsonData = m_readBuffer.mid(headerEnd, contentLength);
        m_readBuffer.remove(0, headerEnd + contentLength);

        // Parse JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit logMessage(tr("JSON parse error: %1").arg(parseError.errorString()));
            continue;
        }

        handleMessage(doc.object());
    }
}

void JsonRpcClient::handleMessage(const QJsonObject& message)
{
    // Check if it's a response
    if (message.contains("id") && !message.contains("method")) {
        int id = message["id"].toInt();

        if (m_pendingRequests.contains(id)) {
            ResponseCallback callback = m_pendingRequests.take(id);

            if (message.contains("error")) {
                callback(QJsonValue(), message["error"].toObject());
            } else {
                // Pass the raw result value - could be array, object, or other type
                callback(message["result"], QJsonObject{});
            }
        }
    }
    // Check if it's a notification
    else if (message.contains("method") && !message.contains("id")) {
        QString method = message["method"].toString();
        QJsonObject params = message["params"].toObject();
        emit notificationReceived(method, params);
    }
}

void JsonRpcClient::writeMessage(const QJsonObject& message)
{
    if (!isRunning()) {
        return;
    }

    QJsonDocument doc(message);
    QByteArray json = doc.toJson(QJsonDocument::Compact);

    // Write with Content-Length header
    QByteArray header = QString("Content-Length: %1\r\n\r\n").arg(json.size()).toUtf8();
    m_process->write(header);
    m_process->write(json);
}

} // namespace XXMLStudio
