#ifndef JSONRPCCLIENT_H
#define JSONRPCCLIENT_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include <functional>

namespace XXMLStudio {

/**
 * JSON-RPC client for communicating with the LSP server over stdio.
 * Handles the Content-Length header protocol.
 */
class JsonRpcClient : public QObject
{
    Q_OBJECT

public:
    // Response callback receives the raw QJsonValue to handle both arrays and objects
    using ResponseCallback = std::function<void(const QJsonValue& result, const QJsonObject& error)>;

    explicit JsonRpcClient(QObject* parent = nullptr);
    ~JsonRpcClient();

    // Start the LSP server process
    bool start(const QString& serverPath, const QStringList& arguments = {});

    // Stop the server
    void stop();

    // Check if server is running
    bool isRunning() const;

    // Send a request (expects a response)
    int sendRequest(const QString& method, const QJsonObject& params, ResponseCallback callback);

    // Send a notification (no response expected)
    void sendNotification(const QString& method, const QJsonObject& params);

signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    void notificationReceived(const QString& method, const QJsonObject& params);
    void logMessage(const QString& message);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    void processIncomingData();
    void handleMessage(const QJsonObject& message);
    void writeMessage(const QJsonObject& message);

    QProcess* m_process = nullptr;
    QByteArray m_readBuffer;
    int m_nextRequestId = 1;
    QMap<int, ResponseCallback> m_pendingRequests;
};

} // namespace XXMLStudio

#endif // JSONRPCCLIENT_H
