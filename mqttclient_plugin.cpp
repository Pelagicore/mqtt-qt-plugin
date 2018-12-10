/* Copyright (C) Luxoft Sweden AB 2018 */

#include "mqttclient_plugin.h"
#include "mosquittoclient.h"
#include <qqml.h>

void MqttclientPlugin::registerTypes(const char *uri)
{
    // @uri mqttclient
    qmlRegisterType<MosquittoClient>(uri, 1, 0, "MosquittoClient");
    qRegisterMetaType<MosquittoClient::ClientState>("ClientState");
    qRegisterMetaType<MosquittoClient::ProtocolVersion>("ProtocolVersion");
    qRegisterMetaType<MosquittoClient::ErrorType>("ErrorType");
}

