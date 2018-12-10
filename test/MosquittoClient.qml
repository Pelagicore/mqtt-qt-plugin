/* Copyright (C) Luxoft Sweden AB 2018 */


import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Controls.Material 2.3

import mqttclient 1.0

Item {
    id: root

    width: 640
    height: 480

    MosquittoClient {
        id: mosquittoSubscriber
        clientId: "SubscriberId"
        hostname: "localhost"
        port: 1883
        keepAlive: 60
        protocolVersion: MosquittoClient.MQTT_3_1_1
        willQos: 0
        willTopic: "subscriber/status"
        willMessage: "offline"
        willRetain: false

        onMessageReceived: {
            messageModel.append({"messageTopic": messageTopic, "messagePayload": messagePayload.toString()})
        }

        onStateChanged: {
            if (state === MosquittoClient.Connected) {
                mosquittoSubscriber.publish("subscriber/status", "online", 0, false)
            }
        }

        onErrorStringChanged: console.log(errorString)
    }

    MosquittoClient {
        id: mosquittoPublisher
        clientId: "PublisherId"
        hostname: "localhost"
        port: 1883
        keepAlive: 60
        protocolVersion: MosquittoClient.MQTT_3_1_1
        willQos: 0
        willTopic: "publisher/status"
        willMessage: "offline"
        willRetain: false

        onStateChanged: {
            if (state === MosquittoClient.Connected) {
                mosquittoPublisher.publish("publisher/status", "online", 0, false)
            }
        }

        onErrorStringChanged: console.log(errorString)
    }

    ListModel {
        id: messageModel
    }


    Pane {
        id: publisherSide
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: parent.width/2 - 10
        Material.elevation: 3

        Column {
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 15
            Label {
                text: "Publisher"
                font.pixelSize: 18
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: publisherSide.width*0.5
                text: {
                    if (mosquittoPublisher.state === MosquittoClient.Disconnected) {
                        return "Connect"
                    } else if (mosquittoPublisher.state === MosquittoClient.Connected) {
                        return "Disconnect"
                    } else {
                        return "Connecting"
                    }
                }
                enabled: mosquittoPublisher.state != MosquittoClient.Connecting
                onClicked: {
                    if (mosquittoPublisher.state === MosquittoClient.Disconnected) {
                        mosquittoPublisher.connect()
                    } else {
                        mosquittoPublisher.disconnect()
                    }
                }
            }
            TextField {
                id: topicText
                anchors.horizontalCenter: parent.horizontalCenter
                width: publisherSide.width*0.5
                placeholderText: "Topic to publish"
                enabled: mosquittoPublisher.state === MosquittoClient.Connected
            }
            TextField {
                id: payloaderText
                anchors.horizontalCenter: parent.horizontalCenter
                width: publisherSide.width*0.5
                placeholderText: "Payloader"
                enabled: mosquittoPublisher.state === MosquittoClient.Connected
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: publisherSide.width*0.5
                text: "Publish"
                enabled: (mosquittoPublisher.state === MosquittoClient.Connected) &&
                         (topicText.text !== "") &&
                         (payloaderText.text !== "")
                onClicked: {
                    mosquittoPublisher.publish(topicText.text, payloaderText.text)
                }
            }
        }
    }

    Pane {
        id: subscriberSide
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 5
        width: parent.width/2 - 10
        Material.elevation: 3

        Column {
            anchors.top: parent.top
            anchors.topMargin: 5
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 15
            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                font.pixelSize: 18
                text: "Subscriber"
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: subscriberSide.width*0.5
                text: {
                    if (mosquittoSubscriber.state === MosquittoClient.Disconnected) {
                        return "Connect"
                    } else if (mosquittoSubscriber.state === MosquittoClient.Connected) {
                        return "Disconnect"
                    } else {
                        return "Connecting"
                    }
                }
                enabled: mosquittoSubscriber.state != MosquittoClient.Connecting
                onClicked: {
                    if (mosquittoSubscriber.state == MosquittoClient.Disconnected) {
                        mosquittoSubscriber.connect()
                    } else {
                        mosquittoSubscriber.disconnect()
                    }
                }
            }
            TextField {
                id: topicTextToSubscribe
                anchors.horizontalCenter: parent.horizontalCenter
                width: publisherSide.width*0.5
                placeholderText: "Topic to subscribe"
                enabled: mosquittoSubscriber.state === MosquittoClient.Connected
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: subscriberSide.width*0.5
                text: "Subscribe"
                enabled: mosquittoSubscriber.state === MosquittoClient.Connected
                onClicked: mosquittoSubscriber.subscribe(topicTextToSubscribe.text)
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                width: subscriberSide.width*0.5
                text: "Unsubscribe"
                enabled: mosquittoSubscriber.state === MosquittoClient.Connected
                onClicked: mosquittoSubscriber.unsubscribe(topicTextToSubscribe.text)
            }
            ListView {
                anchors.horizontalCenter: parent.horizontalCenter
                width: subscriberSide.width*0.9
                height: 200
                model: messageModel
                spacing: 5
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 50
                    border.color: "grey"
                    radius: 5
                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 5
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5
                        Label {
                            text: "Topic: " + model.messageTopic
                        }
                        Label {
                            text: "Payload: " + model.messagePayload
                        }
                    }
                }
            }
        }
    }

}
