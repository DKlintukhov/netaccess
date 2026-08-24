import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: loginPage

    required property var apiClient
    required property var sessionState

    property int loginReqId: 0

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 16
        width: 320

        Label {
            text: "netaccess"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: hostField
            placeholderText: qsTr("Host")
            text: "localhost"
            Layout.fillWidth: true
        }

        TextField {
            id: portField
            placeholderText: qsTr("Port")
            text: "9988"
            Layout.fillWidth: true
            validator: IntValidator { bottom: 1; top: 65535 }
        }

        TextField {
            id: usernameField
            placeholderText: qsTr("Username")
            Layout.fillWidth: true
        }

        TextField {
            id: passwordField
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
            Layout.fillWidth: true
        }

        Button {
            text: qsTr("Connect & Login")
            Layout.fillWidth: true
            enabled: !apiClient.busy
            onClicked: {
                apiClient.connectToServer(hostField.text, parseInt(portField.text))
            }
        }

        Label {
            id: statusLabel
            text: apiClient.connected ? qsTr("Connected") : qsTr("Disconnected")
            color: apiClient.connected ? "green" : "red"
            Layout.alignment: Qt.AlignHCenter
        }

        Connections {
            target: apiClient
            function onConnectedToServer() {
                loginReqId = 1
                apiClient.sendRequest("AUTH", loginReqId, {
                    "username": usernameField.text,
                    "password": passwordField.text
                })
            }
            function onDisconnectedFromServer() {
                statusLabel.text = qsTr("Disconnected")
                statusLabel.color = "red"
            }
        }

        Connections {
            target: loginPage
            function onHandleResponse(reqId, response) {
                if (reqId !== loginReqId) return
                if (response.status === "ok") {
                    var data = response.data
                    sessionState.setAuth(
                        data.token,
                        data.user.id,
                        data.user.username,
                        data.user.full_name,
                        data.user.role,
                        data.user.clearance_level,
                        data.user.department || ""
                    )
                } else {
                    errorLabel.text = response.message || response.code
                    errorDialog.open()
                }
            }
        }
    }

    signal handleResponse(int reqId, var response)
}
