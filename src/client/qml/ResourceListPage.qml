import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: resourcePage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property var resourceModel: []

    Component.onCompleted: loadResources()

    function loadResources() {
        listReqId = 1000 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_LIST", listReqId, {
            "page": 1,
            "page_size": 50
        })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: "Resources"
                font.bold: true
                font.pixelSize: 18
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Refresh")
                font.pointSize: 12
                onClicked: loadResources()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: resourceModel
            clip: true

            delegate: Rectangle {
                width: listView.width
                height: 60
                border.color: "#ddd"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8

                    RowLayout {
                        Label {
                            text: modelData.name || ""
                            font.bold: true
                        }
                        Label {
                            text: modelData.resource_type || ""
                            color: "#666"
                        }
                        Label {
                            text: modelData.address || ""
                            color: "#999"
                            Layout.fillWidth: true
                        }
                    }
                    Label {
                        text: modelData.description || ""
                        color: "#666"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            BusyIndicator {
                anchors.centerIn: parent
                running: client.busy
            }
        }

        Label {
            text: qsTr("Total: ") + (resourceModel.length || 0)
            color: "#666"
        }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (reqId !== listReqId) return
        if (response.status === "ok" && response.data) {
            resourceModel = response.data.items || []
        }
    }
}
