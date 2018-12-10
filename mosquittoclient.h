/* Copyright (C) Luxoft Sweden AB 2018 */

#ifndef MOSQUITTOCLIENT_H
#define MOSQUITTOCLIENT_H

#include <QObject>
#include <QUrl>
#include <mosquitto.h>

class MosquittoClient : public QObject
{
    Q_OBJECT

    // Client's identifier value
    Q_PROPERTY(QString clientId READ clientId WRITE setClientId NOTIFY clientIdChanged)

    // Hostname of the MQTT broker to connect to
    Q_PROPERTY(QString hostname READ hostname WRITE setHostname NOTIFY hostnameChanged)

    // Path to ca certificates
    Q_PROPERTY(QUrl caCertificates READ caCertificates WRITE setCaCertificates NOTIFY caCertificatesChanged)

    // Path to local certificate
    Q_PROPERTY(QUrl localCertificate READ localCertificate WRITE setLocalCertificate NOTIFY localCertificateChanged)

    // Path to client private key
    Q_PROPERTY(QUrl privateKey READ privateKey WRITE setPrivateKey NOTIFY privateKeyChanged)

    // Port. Use 8883 for encrypted connection, otherwise 1883
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

    // Ping interval in seconds
    Q_PROPERTY(int keepAlive READ keepAlive WRITE setKeepAlive NOTIFY keepAliveChanged)

    // Protocol version of the MQTT standard to use during communication with a broker
    Q_PROPERTY(ProtocolVersion protocolVersion READ protocolVersion WRITE setProtocolVersion NOTIFY protocolVersionChanged)

    // Current connection state
    Q_PROPERTY(ClientState state READ state NOTIFY stateChanged)

    // If connected without certificates, give a username
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)

    // If connected without certificates, give a password
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

    // The level of QoS for sending and storing the Will Message
    Q_PROPERTY(int willQos READ willQos WRITE setWillQos NOTIFY willQosChanged)

    // Will topic
    Q_PROPERTY(QString willTopic READ willTopic WRITE setWillTopic NOTIFY willTopicChanged)

    // The payload of a Will Message
    Q_PROPERTY(QByteArray willMessage READ willMessage WRITE setWillMessage NOTIFY willMessageChanged)

    // The property holds if Will Message should be retained on the broker for future subscribers to receive
    Q_PROPERTY(bool willRetain READ willRetain WRITE setWillRetain NOTIFY willRetainChanged)

    // Current error description
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    // Current error code
    Q_PROPERTY(ErrorType error READ error NOTIFY errorChanged)

    // Set to true to instruct the broker to clean all messages and subscriptions on disconnect, false to instruct it to keep them.
    Q_PROPERTY(bool cleanSession READ cleanSession WRITE setCleanSession NOTIFY cleanSessionChanged)

    // Holds a password to decrypt the keyfile
    Q_PROPERTY(QString keyfilePassword READ keyfilePassword WRITE setKeyfilePassword NOTIFY keyfilePasswordChanged)

public:

    explicit MosquittoClient(QObject *parent = nullptr);
    virtual ~MosquittoClient();

    // The protocol version of the MQTT standard which is used to communicate with the broker
    enum ProtocolVersion {
        MQTT_3_1 = 3,
        MQTT_3_1_1 = 4
    };
    Q_ENUM(ProtocolVersion)

    // The client connection state
    enum ClientState {
        Disconnected = 0,
        Connecting,
        Connected
    };
    Q_ENUM(ClientState)

    // Error types
    enum ErrorType {
        ERR_SUCCESS,
        ERR_NOMEM,
        ERR_PROTOCOL,
        ERR_INVAL,
        ERR_NO_CONN,
        ERR_CONN_REFUSED,
        ERR_NOT_FOUND,
        ERR_CONN_LOST,
        ERR_TLS,
        ERR_PAYLOAD_SIZE,
        ERR_NOT_SUPPORTED,
        ERR_UNKNOWN
    };
    Q_ENUM(ErrorType)

    // Connect Qml client
    Q_INVOKABLE void connect();

    // Disconnect Qml client
    Q_INVOKABLE void disconnect();

    // Publish a topic
    Q_INVOKABLE int publish(const QString &topic, const QVariant &payload, const int qos = 0, const bool retain = false);

    // Subscribe a topic
    Q_INVOKABLE void subscribe(const QString &topic, const int qos = 0);

    // Unsubscribe from a topic
    Q_INVOKABLE void unsubscribe(const QString &topic);

    QString clientId() const { return m_clientId; }
    QString hostname() const { return m_hostname; }
    QUrl caCertificates() const { return m_caCertificates; }
    QUrl localCertificate() const { return m_localCertificate; }
    QUrl privateKey() const { return m_privateKey; }
    int port() const { return m_port; }
    int keepAlive() const { return m_keepAlive; }
    ProtocolVersion protocolVersion() const { return m_protocolVersion; }
    ClientState state() const { return m_state; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }
    int willQos() const { return m_willQos; }
    QString willTopic() const { return m_willTopic; }
    QByteArray willMessage() const { return m_willMessage; }
    bool willRetain() const { return m_willRetain; }
    QString errorString() const { return m_errorString; }
    ErrorType error() const { return m_error; }
    bool cleanSession() const { return m_cleanSession; }
    QString keyfilePassword() const { return m_keyfilePassword; }

public slots:
    void setClientId(const QString& clientId);
    void setHostname(const QString& hostname);
    void setCaCertificates(const QUrl& caCertificates);
    void setLocalCertificate(const QUrl& localCertificate);
    void setPrivateKey(const QUrl& privateKey);
    void setPort(const int port);
    void setKeepAlive(const int keepAlive);
    void setProtocolVersion(const ProtocolVersion protocolVersion);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setWillQos(const int willQos);
    void setWillTopic(const QString& willTopic);
    void setWillMessage(const QByteArray willMessage);
    void setWillRetain(const bool willRetain);
    void setCleanSession(const bool cleanSession);
    void setKeyfilePassword(const QString& keyfilePassword);

signals:
    void clientIdChanged(QString& clientId);
    void hostnameChanged(QString& hostname);
    void caCertificatesChanged(QUrl caCertificates);
    void localCertificateChanged(QUrl localCertificate);
    void privateKeyChanged(QUrl privateKey);
    void portChanged(int port);
    void keepAliveChanged(int keepAlive);
    void protocolVersionChanged(ProtocolVersion protocolVersion);
    void stateChanged(ClientState state);
    void usernameChanged(QString& username);
    void passwordChanged(QString& password);
    void willQosChanged(int willQos);
    void willTopicChanged(QString& willTopic);
    void willMessageChanged(QByteArray willMessage);
    void willRetainChanged(bool willRetain);
    // Deliver the received topic and payload from the broker to the Qml client
    void messageReceived(QString messageTopic, QByteArray messagePayload, int messageQoS, bool messageRetain);
    void subscribed();
    void published(int messageId);
    void errorStringChanged(QString errorString);
    void errorChanged(ErrorType error);
    void unsubscribed();
    void cleanSessionChanged(bool cleanSession);
    void keyfilePasswordChanged(QString& keyfilePassword);

private:
    static void messageCallback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
    static void connectCallback(struct mosquitto *mosq, void *userdata, int result);
    static void subscribeCallback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos);
    static void publishCallback(struct mosquitto *mosq, void *userdata, int mid);
    static void logCallback(struct mosquitto *mosq, void *userdata, int level, const char *str);
    static void disconnectCallback(struct mosquitto *mosq, void *userdata, int result);
    static void unsubscribeCallback(struct mosquitto *mosq, void *userdata, int mid);
    static int keyfilePasswordCallback(char *buf, int size, int rwflag, void *userdata);

    void setState(const ClientState state);
    void setTlsConfiguration();
    void mosquittoLoopStart();
    void setError(const ErrorType error, const QString &errorString);

    struct mosquitto *mosq = nullptr;
    QString m_clientId;
    QString m_hostname;
    QUrl m_caCertificates;
    QUrl m_localCertificate;
    QUrl m_privateKey;
    int m_port = 1883;
    int m_keepAlive = 60;
    ProtocolVersion m_protocolVersion = ProtocolVersion::MQTT_3_1_1;
    ClientState m_state = ClientState::Disconnected;
    QString m_username;
    QString m_password;
    int m_willQos = 0;
    QString m_willTopic;
    QByteArray m_willMessage;
    bool m_willRetain = false;
    QString m_errorString;
    ErrorType m_error = ErrorType::ERR_SUCCESS;
    bool m_cleanSession = true;
    QString m_keyfilePassword;
};

#endif // MOSQUITTOCLIENT_H
