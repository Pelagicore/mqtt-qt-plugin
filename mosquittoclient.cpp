/* Copyright (C) Luxoft Sweden AB 2018 */

#include "mosquittoclient.h"
#include <QtCore/QLoggingCategory>
#include <QFile>

Q_LOGGING_CATEGORY(mosquittoClientLogging, "mosquitto.client", QtWarningMsg)

void MosquittoClient::messageCallback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    Q_UNUSED(mosq);
    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        // http://doc.qt.io/qt-5/qtqml-cppintegration-data.html#qbytearray-to-javascript-arraybuffer
        // QByteArray will be automatically converted into JavaScript ArrayBuffer
        QByteArray baPayload(static_cast<char*>(message->payload), message->payloadlen);
        emit client->messageReceived(message->topic, baPayload, message->qos, message->retain);
    }
}

void MosquittoClient::connectCallback(struct mosquitto *mosq, void *userdata, int result)
{
    Q_UNUSED(mosq);

    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        if (result == 0) {
            client->setState(ClientState::Connected);
            return;
        } else if (result == 1) {
            client->setError(ErrorType::ERR_PROTOCOL, "Unacceptable protocol version");
        } else if (result == 2) {
            client->setError(ErrorType::ERR_INVAL, "Identifier rejected");
        } else if (result == 3) {
            client->setError(ErrorType::ERR_CONN_REFUSED, "Broker unavailable");
        } else {
            client->setError(ErrorType::ERR_UNKNOWN, QString("Unknown error when connecting: %1").arg(strerror(errno)));
        }
        client->setState(ClientState::Disconnected);
    }
}

void MosquittoClient::subscribeCallback(struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    Q_UNUSED(mosq);
    Q_UNUSED(mid);
    Q_UNUSED(qos_count);
    Q_UNUSED(granted_qos);

    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        emit client->subscribed();
    }
}

void MosquittoClient::publishCallback(struct mosquitto *mosq, void *userdata, int mid)
{
    Q_UNUSED(mosq);
    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        emit client->published( mid );
    }
}

void MosquittoClient::logCallback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    Q_UNUSED(mosq);
    Q_UNUSED(userdata);
    Q_UNUSED(level);
    /* Print all log messages regardless of level. */
    qCDebug(mosquittoClientLogging) << str;
}

void MosquittoClient::disconnectCallback(mosquitto *mosq, void *userdata, int result)
{
    Q_UNUSED(mosq);
    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        if (result == 0) {
            client->setState(ClientState::Disconnected);
        } else {
            QString errMessage = QString("%1 disconnected unexpectedly").arg(client->clientId());
            client->setError(ErrorType::ERR_NO_CONN, errMessage);
        }
    }
}

void MosquittoClient::unsubscribeCallback(mosquitto *mosq, void *userdata, int mid)
{
    Q_UNUSED(mosq);
    Q_UNUSED(mid);
    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        emit client->unsubscribed();
    }
}

int MosquittoClient::keyfilePasswordCallback(char *buf, int size, int rwflag, void *userdata)
{
    Q_UNUSED(rwflag);
    int retPasswordLength = 0;
    MosquittoClient *client = reinterpret_cast<MosquittoClient*>(userdata);
    if (client) {
        retPasswordLength = client->m_keyfilePassword.length();
        if (retPasswordLength > size) {
            client->setError(ERR_INVAL, "The keyfile password is too long");
            return 0;
        }
        memcpy(buf, client->m_keyfilePassword.toStdString().c_str(), static_cast<size_t>(retPasswordLength) );
    }
    return retPasswordLength;
}

MosquittoClient::MosquittoClient(QObject *parent) : QObject(parent)
{
    mosquitto_lib_init();
}

MosquittoClient::~MosquittoClient()
{
    if (mosq) {
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
    }
}


void MosquittoClient::setTlsConfiguration()
{
    if ( m_caCertificates.isEmpty() && m_localCertificate.isEmpty() && m_privateKey.isEmpty() ) {
        return;
    }

    if ( (!m_caCertificates.isEmpty()) && (!QFile::exists(m_caCertificates.toLocalFile())) ) {
        setError(ErrorType::ERR_NOT_FOUND, QString("%1 not found").arg(m_caCertificates.toLocalFile()));
        return;
    }

    if ( (!m_localCertificate.isEmpty()) && (!QFile::exists(m_localCertificate.toLocalFile())) ) {
        setError(ErrorType::ERR_NOT_FOUND, QString("%1 not found").arg(m_localCertificate.toLocalFile()));
        return;
    }

    if ( (!m_privateKey.isEmpty()) && (!QFile::exists(m_privateKey.toLocalFile())) ) {
        setError(ErrorType::ERR_NOT_FOUND, QString("%1 not found").arg(m_privateKey.toLocalFile()));
        return;
    }

    if (mosq) {
        int retCode = mosquitto_tls_set(mosq,
                                        m_caCertificates.path().toStdString().c_str(),
                                        nullptr,
                                        m_localCertificate.path().toStdString().c_str(),
                                        m_privateKey.path().toStdString().c_str(),
                                        MosquittoClient::keyfilePasswordCallback);
        if (retCode == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_TLS, "TLS configuration is invalid.");
        } else if (retCode == mosq_err_t::MOSQ_ERR_NOMEM) {
            setError(ErrorType::ERR_NOMEM, "An out of memory condition occurred when setting certificates.");
        } else if (retCode == mosq_err_t::MOSQ_ERR_UNKNOWN) {
            setError(ErrorType::ERR_UNKNOWN, "Unknown error to set certificates");
        }
    } else {
        setError(ErrorType::ERR_NOMEM, "No valid mosquitto instance");
    }
}

void MosquittoClient::mosquittoLoopStart()
{
    if (mosq) {
        int retCode = mosquitto_loop_start(mosq);
        if (retCode == mosq_err_t::MOSQ_ERR_SUCCESS) {
            return;
        } else if (retCode == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_INVAL, "Error to start mosquitto loop");
        } else if (retCode == mosq_err_t::MOSQ_ERR_NOT_SUPPORTED) {
            setError(ErrorType::ERR_NOT_SUPPORTED, "No support for threads");
        } else {
            setError(ErrorType::ERR_UNKNOWN, "Unknown error to start mosquitto loop");
        }
    } else {
        setError(ErrorType::ERR_NOMEM, "No valid mosquitto instance");
    }
}

void MosquittoClient::setError(const ErrorType error, const QString &errorString)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged(m_error);

    qCWarning(mosquittoClientLogging) << errorString;
    m_errorString = errorString;
    emit errorStringChanged(errorString);
}

void MosquittoClient::connect()
{
    qCDebug(mosquittoClientLogging) << Q_FUNC_INFO << QString("Connecting to %1:%2").arg(m_hostname).arg(m_port);
    mosq = mosquitto_new(m_clientId.toStdString().c_str(), m_cleanSession, this);

    if (mosq) {
        mosquitto_log_callback_set(mosq, logCallback);
        mosquitto_connect_callback_set(mosq, connectCallback);
        mosquitto_message_callback_set(mosq, messageCallback );
        mosquitto_publish_callback_set(mosq, publishCallback);
        mosquitto_subscribe_callback_set(mosq, subscribeCallback);
        mosquitto_disconnect_callback_set(mosq, disconnectCallback);
        mosquitto_unsubscribe_callback_set(mosq, unsubscribeCallback);
        setState(ClientState::Connecting);

        if (!m_willTopic.isEmpty()) {
            if (mosquitto_will_set(mosq, m_willTopic.toStdString().c_str(), m_willMessage.length(), m_willMessage, m_willQos, m_willRetain)) {
                setError(ErrorType::ERR_INVAL, "Error to set the last will topic.");
            }
        }

        setTlsConfiguration();

        if (!m_username.isEmpty() && !m_password.isEmpty()) {
            int result = mosquitto_username_pw_set(mosq, m_username.toStdString().c_str(), m_password.toStdString().c_str());
            if (result == mosq_err_t::MOSQ_ERR_INVAL) {
                setError(ErrorType::ERR_INVAL, "Invalid parameters for username or password");
            } else if (result == mosq_err_t::MOSQ_ERR_NOMEM) {
                setError(ErrorType::ERR_NOMEM, "An out of memory condition occurred in mosquitto_username_pw_set");
            } else if (result != mosq_err_t::MOSQ_ERR_SUCCESS) {
                setError(ErrorType::ERR_UNKNOWN, "Unknown error in mosquitto_username_pw_set");
            }
        }

        int connResult = mosquitto_connect_async(mosq, m_hostname.toStdString().c_str(), m_port, m_keepAlive);
        if (connResult == mosq_err_t::MOSQ_ERR_SUCCESS) {
            mosquittoLoopStart();
        } else if (connResult == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_INVAL, "Invalid parameters for username or password");
        } else if (connResult == mosq_err_t::MOSQ_ERR_ERRNO) {
            setError(ErrorType::ERR_UNKNOWN, QString("system call returned an error: %1").arg(strerror(errno)));
        } else {
            setError(ErrorType::ERR_INVAL, QString("Unable to initiate connection to %1").arg(m_hostname));
            setState(ClientState::Disconnected);
        }
    }
}

void MosquittoClient::disconnect()
{
    if (m_state == ClientState::Disconnected) {
        return;
    }
    if (mosq) {
        int result = mosquitto_disconnect(mosq);
        if (result == mosq_err_t::MOSQ_ERR_SUCCESS) {
            int retVal = mosquitto_loop_stop(mosq, false);
            if (retVal == mosq_err_t::MOSQ_ERR_SUCCESS) {
                return;
            } else if (retVal == mosq_err_t::MOSQ_ERR_INVAL) {
                setError(ErrorType::ERR_INVAL, "Invalid parameters to disconnect");
            } else if (retVal == mosq_err_t::MOSQ_ERR_NOT_SUPPORTED) {
                setError(ErrorType::ERR_NOT_SUPPORTED, "No support for threads");
            } else {
                setError(ErrorType::ERR_UNKNOWN, "Unknown error");
            }
        } else if (result == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_INVAL, "Invalid parameters to disconnect");
        } else if (result == mosq_err_t::MOSQ_ERR_NO_CONN) {
            setError(ErrorType::ERR_NO_CONN, "No connection");
        } else {
            setError(ErrorType::ERR_UNKNOWN, "Unknown error to disconnect");
        }
    } else {
        setError(ErrorType::ERR_NOMEM, "No valid mosquitto instance");
    }
}

int MosquittoClient::publish(const QString &topic, const QVariant &payload, const int qos, const bool retain)
{
    qCDebug(mosquittoClientLogging) << Q_FUNC_INFO << "Publish a message to topic:" << topic;
    if (mosquitto_pub_topic_check(topic.toStdString().c_str()) == mosq_err_t::MOSQ_ERR_INVAL) {
        setError(ErrorType::ERR_INVAL, QString("Topic %1 is not valid").arg(topic));
    }
    int mid = -1;
    if (mosq) {
        if (qos < 0 || qos > 2) {
            setError(ErrorType::ERR_INVAL, "Give a valid value for QoS: 0, 1 or 2.");
            return mid;
        }
        int retCode = mosquitto_publish(mosq, &mid, topic.toStdString().c_str(), payload.toByteArray().length(), payload.toByteArray(), qos, retain);
        if (retCode == mosq_err_t::MOSQ_ERR_SUCCESS) {
            return mid;
        } else if (retCode == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_INVAL, "Invalid parameters");
        } else if (retCode == mosq_err_t::MOSQ_ERR_NOMEM) {
            setError(ErrorType::ERR_NOMEM, "An out of memory condition occurred in mosquitto_publish");
        } else if (retCode == mosq_err_t::MOSQ_ERR_NO_CONN) {
            setError(ErrorType::ERR_NO_CONN, "No connection");
        } else if (retCode == mosq_err_t::MOSQ_ERR_PROTOCOL) {
            setError(ErrorType::ERR_PROTOCOL, "Wrong protocol version");
        } else if (retCode == mosq_err_t::MOSQ_ERR_PAYLOAD_SIZE) {
            setError(ErrorType::ERR_PAYLOAD_SIZE, "Incorrect size of payload");
        } else {
            setError(ErrorType::ERR_UNKNOWN, "Unknown error");
        }
    } else {
        setError(ErrorType::ERR_NOMEM, "No valid mosquitto instance");
    }

    return mid;
}

void MosquittoClient::subscribe(const QString &topic, const int qos)
{
    if (topic.isEmpty()) {
        return;
    } else if (qos < 0 || qos > 2) {
        setError(ErrorType::ERR_INVAL, QString("Give a valid value for QoS: 0, 1 or 2, when subscribing a topic %1").arg(topic));
        return;
    } else if (mosquitto_sub_topic_check(topic.toStdString().c_str()) == mosq_err_t::MOSQ_ERR_INVAL) {
        setError(ErrorType::ERR_INVAL, QString("Invalid topic %1").arg(topic));
    } else if (mosq) {
        qCDebug(mosquittoClientLogging) << QString("Subscribing to a new topic %1").arg(topic);
        int result = mosquitto_subscribe(mosq, nullptr, topic.toStdString().c_str(), qos);
        if (result == mosq_err_t::MOSQ_ERR_SUCCESS) {
            return;
        } else if (result == mosq_err_t::MOSQ_ERR_INVAL) {
            setError(ErrorType::ERR_INVAL, QString("Invalid parameters to subscribe to %1").arg(topic));
        }
        else if (result == mosq_err_t::MOSQ_ERR_NOMEM) {
            setError(ErrorType::ERR_NOMEM, QString("An out of memory condition occurred when subsribing a topic %1").arg(topic));
        }
        else if (result == mosq_err_t::MOSQ_ERR_NO_CONN) {
            setError(ErrorType::ERR_NO_CONN, QString("No connection to subscribe to %1").arg(topic));
        } else {
            setError(ErrorType::ERR_UNKNOWN, QString("Unknown error when subscribing to %1").arg(topic));
        }
    } else {
        setError(ErrorType::ERR_NOMEM, QString("Out of memory, no valid mosquitto instance when subscribing to %1").arg(topic));
    }
}

void MosquittoClient::unsubscribe(const QString &topic)
{
    if (mosq) {
        if (topic.isEmpty()) {
            return;
        }
        qCDebug(mosquittoClientLogging) << QString("Unsubscribing from topic %1").arg(topic);
        int result = mosquitto_unsubscribe(mosq, nullptr, topic.toStdString().c_str());
        if (result == mosq_err_t::MOSQ_ERR_SUCCESS) {
            return;
        } else if (result == mosq_err_t::MOSQ_ERR_INVAL)
            setError(ErrorType::ERR_INVAL, QString("Invalid parameters to unsubscribe from %1").arg(topic));
        else if (result == mosq_err_t::MOSQ_ERR_NOMEM) {
            setError(ErrorType::ERR_NOMEM, QString("An out of memory condition occurred when unsubsribing from topic %1").arg(topic));
        } else if (result == mosq_err_t::MOSQ_ERR_NO_CONN) {
            setError(ErrorType::ERR_NO_CONN, QString("No connection to unsubscribe from %1").arg(topic));
        } else {
            setError(ErrorType::ERR_UNKNOWN, QString("Unknown error when unsubscribing from %1").arg(topic));
        }
    } else {
        setError(ErrorType::ERR_NOMEM, QString("Out of memory, no valid mosquitto instance when unsubscribing from %1").arg(topic));
    }
}

void MosquittoClient::setClientId(const QString& clientId)
{
    if (m_clientId == clientId)
        return;

    m_clientId = clientId;
    emit clientIdChanged(m_clientId);
}

void MosquittoClient::setHostname(const QString& hostname)
{
    if (m_hostname == hostname)
        return;

    m_hostname = hostname;
    emit hostnameChanged(m_hostname);
}

void MosquittoClient::setCaCertificates(const QUrl& caCertificates)
{
    if (m_caCertificates == caCertificates)
        return;

    m_caCertificates = caCertificates;
    emit caCertificatesChanged(m_caCertificates);
}

void MosquittoClient::setLocalCertificate(const QUrl& localCertificate)
{
    if (m_localCertificate == localCertificate)
        return;

    m_localCertificate = localCertificate;
    emit localCertificateChanged(m_localCertificate);
}

void MosquittoClient::setPrivateKey(const QUrl& privateKey)
{
    if (m_privateKey == privateKey)
        return;

    m_privateKey = privateKey;
    emit privateKeyChanged(m_privateKey);
}

void MosquittoClient::setPort(const int port)
{
    if (m_port == port)
        return;

    m_port = port;
    emit portChanged(m_port);
}

void MosquittoClient::setKeepAlive(const int keepAlive)
{
    if (m_keepAlive == keepAlive)
        return;

    m_keepAlive = keepAlive;
    emit keepAliveChanged(m_keepAlive);
}

void MosquittoClient::setProtocolVersion(const ProtocolVersion protocolVersion)
{
    if (m_protocolVersion == protocolVersion)
        return;

    m_protocolVersion = protocolVersion;
    emit protocolVersionChanged(m_protocolVersion);
}

void MosquittoClient::setState(const ClientState state)
{
    if (m_state == state)
        return;

    m_state = state;
    emit stateChanged(m_state);
}

void MosquittoClient::setUsername(const QString& username)
{
    if (m_username == username)
        return;

    m_username = username;
    emit usernameChanged(m_username);
}

void MosquittoClient::setPassword(const QString& password)
{
    if (m_password == password)
        return;

    m_password = password;
    emit passwordChanged(m_password);
}

void MosquittoClient::setWillQos(const int willQos)
{
    if (m_willQos == willQos)
        return;

    m_willQos = willQos;
    emit willQosChanged(m_willQos);
}

void MosquittoClient::setWillTopic(const QString& willTopic)
{
    if (m_willTopic == willTopic)
        return;

    m_willTopic = willTopic;
    emit willTopicChanged(m_willTopic);
}


void MosquittoClient::setWillMessage(const QByteArray willMessage)
{
    if (m_willMessage == willMessage)
        return;

    m_willMessage = willMessage;
    emit willMessageChanged(m_willMessage);
}

void MosquittoClient::setWillRetain(const bool willRetain)
{
    if (m_willRetain == willRetain)
        return;

    m_willRetain = willRetain;
    emit willRetainChanged(m_willRetain);
}

void MosquittoClient::setCleanSession(const bool cleanSession)
{
    if (m_cleanSession == cleanSession)
        return;

    m_cleanSession = cleanSession;
    emit cleanSessionChanged(m_cleanSession);
}

void MosquittoClient::setKeyfilePassword(const QString& keyfilePassword)
{
    if (m_keyfilePassword == keyfilePassword)
        return;

    m_keyfilePassword = keyfilePassword;
    emit keyfilePasswordChanged(m_keyfilePassword);
}
