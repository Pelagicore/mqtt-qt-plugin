/* Copyright (C) Luxoft Sweden AB 2018 */

#pragma once

#include <QQmlExtensionPlugin>

class MqttclientPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri);
};
