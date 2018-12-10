#!/bin/bash
# Copyright (C) Luxoft Sweden AB 2018

OPEN_SSL=/home/path_to/openssl-1.0.2n
MOSQUITTO_LIB=/home/path_to/mqttclient/lib
export LD_LIBRARY_PATH=$OPEN_SSL:$MOSQUITTO_LIB

#export QT_LOGGING_RULES="*.debug=false;mosquitto.client.debug=true"
export QT_LOGGING_RULES="mosquitto.client.debug=true"

qmlscene -style material ./MosquittoClient.qml
