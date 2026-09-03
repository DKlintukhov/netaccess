import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: userPage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property int createReqId: 0
    property int editReqId: 0
    property int deleteReqId: 0
    property var userModel: []
    property var pendingDelete: null
    property var pendingEdit: null

    Component.onCompleted: loadUsers()

    function loadUsers() {
        listReqId = 2000 + Math.floor(Math.random() * 10000)
        client.sendRequest("USER_LIST", listReqId, {"page": 1, "page_size": 50})
    }

    function createUser() {
        createReqId = 2100 + Math.floor(Math.random() * 10000)
        client.sendRequest("USER_CREATE", createReqId, {
            "username": usernameField.text,
            "password": passwordField.text,
            "full_name": fullNameField.text,
            "role": roleCombo.currentText,
            "department": deptField.text,
            "clearance_level": parseInt(clearanceField.text) || 0
        })
    }

    function deleteUser(id) {
        deleteReqId = 2200 + Math.floor(Math.random() * 10000)
        client.sendRequest("USER_DELETE", deleteReqId, {"user_id": id})
    }

    function openEditDialog(item) {
        pendingEdit = item
        editFullNameField.text = item.full_name || ""
        editRoleCombo.currentIndex = Math.max(0, editRoleCombo.model.indexOf(item.role || ""))
        editDeptField.text = item.department || ""
        editClearanceField.text = (item.clearance_level !== undefined) ? String(item.clearance_level) : "0"
        editPositionField.text = item.position || ""
        editActiveCheck.checked = item.is_active !== false
        editDialog.open()
    }

    function updateUser() {
        editReqId = 2300 + Math.floor(Math.random() * 10000)
        client.sendRequest("USER_UPDATE", editReqId, {
            "user_id": pendingEdit.id,
            "full_name": editFullNameField.text,
            "role": editRoleCombo.currentText,
            "department": editDeptField.text,
            "clearance_level": parseInt(editClearanceField.text) || 0,
            "position": editPositionField.text,
            "is_active": editActiveCheck.checked
        })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr("Users"); font.bold: true; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); font.pointSize: 12; onClicked: loadUsers() }
            Button {
                text: qsTr("Add")
                font.pointSize: 12
                visible: session.isAdmin()
                onClicked: createDialog.open()
            }
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: userModel
            clip: true

            delegate: Rectangle {
                width: parent.width
                height: 50
                border.color: "#ddd"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    Label { text: modelData.username || ""; font.bold: true; width: 120 }
                    Label { text: modelData.full_name || ""; Layout.fillWidth: true }
                    Label { text: modelData.role || ""; color: "#666"; width: 80 }
                    Label { text: modelData.department || ""; color: "#999"; width: 100 }
                    Label { text: modelData.is_active ? qsTr("active") : qsTr("inactive"); color: modelData.is_active ? "green" : "red" }
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
            }

            BusyIndicator { anchors.centerIn: parent; running: client.busy }
        }

        Label { text: qsTr("Total: ") + (userModel.length || 0); color: "#666" }
    }

    Dialog {
        id: createDialog
        title: qsTr("New User")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: createUser()

        ColumnLayout {
            width: 300
            spacing: 8
            TextField { id: usernameField; placeholderText: qsTr("Username"); Layout.fillWidth: true }
            TextField { id: passwordField; placeholderText: qsTr("Password"); echoMode: TextInput.Password; Layout.fillWidth: true }
            TextField { id: fullNameField; placeholderText: qsTr("Full Name"); Layout.fillWidth: true }
            ComboBox {
                id: roleCombo
                model: ["user", "auditor", "admin"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: deptField; placeholderText: qsTr("Department"); Layout.fillWidth: true }
            TextField {
                id: clearanceField
                placeholderText: qsTr("Clearance Level (0-5)")
                validator: IntValidator { bottom: 0; top: 5 }
                Layout.fillWidth: true
            }
        }
    }

    Dialog {
        id: editDialog
        title: qsTr("Edit User")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: updateUser()
        onRejected: pendingEdit = null

        ColumnLayout {
            width: 300
            spacing: 8
            TextField { id: editFullNameField; placeholderText: qsTr("Full Name"); Layout.fillWidth: true }
            ComboBox {
                id: editRoleCombo
                model: ["user", "auditor", "admin"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: editDeptField; placeholderText: qsTr("Department"); Layout.fillWidth: true }
            TextField { id: editPositionField; placeholderText: qsTr("Position"); Layout.fillWidth: true }
            TextField {
                id: editClearanceField
                placeholderText: qsTr("Clearance Level (0-5)")
                validator: IntValidator { bottom: 0; top: 5 }
                Layout.fillWidth: true
            }
            CheckBox { id: editActiveCheck; text: qsTr("Active") }
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
                deleteUser(pendingDelete.id)
            pendingDelete = null
        }
        onRejected: pendingDelete = null

        Label {
            text: pendingDelete ? qsTr("Delete user '%1'?").arg(pendingDelete.username) : ""
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
            userModel = response.data.items || []
        } else if (reqId === createReqId || reqId === editReqId || reqId === deleteReqId) {
            loadUsers()
        }
    }
}
