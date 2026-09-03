import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: policyPage
    background: null

    required property var client
    required property var session

    property int listReqId: 0
    property int createReqId: 0
    property int editReqId: 0
    property int deleteReqId: 0
    property var policyModel: []
    property var pendingDelete: null
    property var pendingEdit: null

    Component.onCompleted: loadPolicies()

    function loadPolicies() {
        listReqId = 3000 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_LIST", listReqId, {"page": 1, "page_size": 50})
    }

    function createPolicy() {
        createReqId = 3100 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_CREATE", createReqId, {
            "name": nameField.text,
            "action": actionCombo.currentText,
            "role_required": roleField.text,
            "department_required": deptField.text,
            "resource_type": resourceTypeField.text,
            "min_clearance": clearanceCombo.currentText === "" ? -1 : parseInt(clearanceCombo.currentText),
            "priority": parseInt(priorityField.text) || 0,
            "enabled": true
        })
    }

    function deletePolicy(id) {
        deleteReqId = 3200 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_DELETE", deleteReqId, {"policy_id": id})
    }

    function openEditDialog(item) {
        pendingEdit = item
        editNameField.text = item.name || ""
        editActionCombo.currentIndex = Math.max(0, editActionCombo.model.indexOf(item.action || "read"))
        editRoleField.text = item.role_required || ""
        editDeptField.text = item.department_required || ""
        editResourceTypeField.text = item.resource_type || ""
        var cl = item.min_clearance !== undefined ? String(item.min_clearance) : ""
        editClearanceCombo.currentIndex = Math.max(0, editClearanceCombo.model.indexOf(cl))
        editPriorityField.text = (item.priority !== undefined) ? String(item.priority) : "0"
        editEnabledCheck.checked = item.enabled !== false
        editDialog.open()
    }

    function updatePolicy() {
        editReqId = 3300 + Math.floor(Math.random() * 10000)
        client.sendRequest("POLICY_UPDATE", editReqId, {
            "policy_id": pendingEdit.id,
            "name": editNameField.text,
            "action": editActionCombo.currentText,
            "role_required": editRoleField.text,
            "department_required": editDeptField.text,
            "resource_type": editResourceTypeField.text,
            "min_clearance": editClearanceCombo.currentText === "" ? -1 : parseInt(editClearanceCombo.currentText),
            "priority": parseInt(editPriorityField.text) || 0,
            "enabled": editEnabledCheck.checked
        })
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr("Policies"); font.bold: true; font.pixelSize: 18 }
            Item { Layout.fillWidth: true }
            Button { text: qsTr("Refresh"); font.pointSize: 12; onClicked: loadPolicies() }
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
            model: policyModel
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
                    Label { text: modelData.name || ""; font.bold: true; width: 180 }
                    Label { text: modelData.action || ""; color: "#666"; width: 80 }
                    Label { text: modelData.role_required || "*"; color: "#999"; width: 80 }
                    Label { text: modelData.resource_type || "*"; color: "#999"; width: 100 }
                    Label { text: "P:" + (modelData.priority || 0); width: 50 }
                    Label { text: modelData.enabled ? qsTr("ON") : qsTr("OFF"); color: modelData.enabled ? "green" : "red" }
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

        Label { text: qsTr("Total: ") + (policyModel.length || 0); color: "#666" }
    }

    Dialog {
        id: createDialog
        title: qsTr("New Policy")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: createPolicy()

        ColumnLayout {
            width: 320
            spacing: 8
            TextField { id: nameField; placeholderText: qsTr("Name"); Layout.fillWidth: true }
            ComboBox {
                id: actionCombo
                model: ["read", "write", "manage", "connect", "select", "modify", "print", "access", "admin", "any"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: roleField; placeholderText: qsTr("Role required (empty = any)"); Layout.fillWidth: true }
            TextField { id: deptField; placeholderText: qsTr("Department required (empty = any)"); Layout.fillWidth: true }
            TextField { id: resourceTypeField; placeholderText: qsTr("Resource type (empty = any)"); Layout.fillWidth: true }
            RowLayout {
                ComboBox {
                    id: clearanceCombo
                    model: ["", "0", "1", "2", "3", "4", "5"]
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 30
                }
                TextField {
                    id: priorityField
                    placeholderText: qsTr("Priority")
                    validator: IntValidator { bottom: 0; top: 99999 }
                    Layout.fillWidth: true
                }
            }
        }
    }

    Dialog {
        id: editDialog
        title: qsTr("Edit Policy")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: updatePolicy()
        onRejected: pendingEdit = null

        ColumnLayout {
            width: 320
            spacing: 8
            TextField { id: editNameField; placeholderText: qsTr("Name"); Layout.fillWidth: true }
            ComboBox {
                id: editActionCombo
                model: ["read", "write", "manage", "connect", "select", "modify", "print", "access", "admin", "any"]
                Layout.fillWidth: true
                Layout.preferredHeight: 30
            }
            TextField { id: editRoleField; placeholderText: qsTr("Role required (empty = any)"); Layout.fillWidth: true }
            TextField { id: editDeptField; placeholderText: qsTr("Department required (empty = any)"); Layout.fillWidth: true }
            TextField { id: editResourceTypeField; placeholderText: qsTr("Resource type (empty = any)"); Layout.fillWidth: true }
            RowLayout {
                ComboBox {
                    id: editClearanceCombo
                    model: ["", "0", "1", "2", "3", "4", "5"]
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 30
                }
                TextField {
                    id: editPriorityField
                    placeholderText: qsTr("Priority")
                    validator: IntValidator { bottom: 0; top: 99999 }
                    Layout.fillWidth: true
                }
            }
            CheckBox { id: editEnabledCheck; text: qsTr("Enabled") }
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
                deletePolicy(pendingDelete.id)
            pendingDelete = null
        }
        onRejected: pendingDelete = null

        Label {
            text: pendingDelete ? qsTr("Delete policy '%1'?").arg(pendingDelete.name) : ""
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
            policyModel = response.data.items || []
        } else if (reqId === createReqId || reqId === editReqId || reqId === deleteReqId) {
            loadPolicies()
        }
    }
}
