import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: resourcePage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property int createReqId: 0
    property int editReqId: 0
    property int deleteReqId: 0
    property var resourceModel: []
    property var pendingDelete: null
    property var pendingEdit: null

    Component.onCompleted: loadResources()

    function loadResources() {
        listReqId = 1000 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_LIST", listReqId, {
            "page": 1,
            "page_size": 50
        })
    }

    function createResource() {
        createReqId = 1100 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_CREATE", createReqId, {
            "name": nameField.text,
            "resource_type": typeCombo.currentText,
            "address": addressField.text,
            "description": descField.text
        })
    }

    function openEditDialog(item) {
        pendingEdit = item
        editNameField.text = item.name || ""
        editTypeCombo.currentIndex = Math.max(0, editTypeCombo.model.indexOf(item.resource_type || ""))
        editAddressField.text = item.address || ""
        editDescField.text = item.description || ""
        editDialog.open()
    }

    function updateResource() {
        editReqId = 1300 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_UPDATE", editReqId, {
            "resource_id": pendingEdit.id,
            "name": editNameField.text,
            "resource_type": editTypeCombo.currentText,
            "address": editAddressField.text,
            "description": editDescField.text
        })
    }

    function deleteResource(id) {
        deleteReqId = 1200 + Math.floor(Math.random() * 10000)
        client.sendRequest("RESOURCE_DELETE", deleteReqId, {"resource_id": id})
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label {
                text: qsTr("Resources")
                font.bold: true
                font.pixelSize: 18
            }
            Item { Layout.fillWidth: true }
            Button {
                text: qsTr("Refresh")
                font.pointSize: 12
                onClicked: loadResources()
            }
            Button {
                text: qsTr("Add")
                font.pointSize: 12
                visible: session.isAdmin()
                onClicked: createDialog.open()
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
                        Button {
                            text: qsTr("Edit")
                            font.pointSize: 10
                            visible: session.isAdmin()
                            onClicked: openEditDialog(modelData)
                        }
                        Button {
                            text: qsTr("Delete")
                            font.pointSize: 10
                            visible: session.isAdmin()
                            onClicked: {
                                pendingDelete = modelData
                                deleteDialog.open()
                            }
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

    Dialog {
        id: createDialog
        title: qsTr("New Resource")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: createResource()

        ColumnLayout {
            width: 300
            spacing: 8
            TextField { id: nameField; placeholderText: qsTr("Name"); Layout.fillWidth: true }
            ComboBox {
                id: typeCombo
                model: ["file_share", "database", "printer", "web_service", "server", "vpn"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: addressField; placeholderText: qsTr("Address"); Layout.fillWidth: true }
            TextField { id: descField; placeholderText: qsTr("Description"); Layout.fillWidth: true }
        }
    }

    Dialog {
        id: editDialog
        title: qsTr("Edit Resource")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: updateResource()
        onRejected: pendingEdit = null

        ColumnLayout {
            width: 300
            spacing: 8
            TextField { id: editNameField; placeholderText: qsTr("Name"); Layout.fillWidth: true }
            ComboBox {
                id: editTypeCombo
                model: ["file_share", "database", "printer", "web_service", "server", "vpn"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: editAddressField; placeholderText: qsTr("Address"); Layout.fillWidth: true }
            TextField { id: editDescField; placeholderText: qsTr("Description"); Layout.fillWidth: true }
        }
    }

    Dialog {
        id: deleteDialog
        title: qsTr("Confirm Delete")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            if (pendingDelete)
                deleteResource(pendingDelete.id)
            pendingDelete = null
        }
        onRejected: pendingDelete = null

        Label {
            text: pendingDelete ? qsTr("Delete resource '%1'?").arg(pendingDelete.name) : ""
            wrapMode: Text.Wrap
        }
    }

    signal handleResponse(int reqId, var response)

    onHandleResponse: function(reqId, response) {
        if (response.status !== "ok") {
            errorLabel.text = response.message || response.code || qsTr("Error")
            errorDialog.open()
            return
        }
        if (reqId === listReqId && response.data) {
            resourceModel = response.data.items || []
        } else if (reqId === createReqId || reqId === editReqId || reqId === deleteReqId) {
            loadResources()
        }
    }
}
