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
            text: "NetAccess"
            font.pixelSize: 26
            clip: false
            font.styleName: "Normal"
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: hostField
            placeholderText: qsTr("Host")
            text: "localhost"
            padding: 4
            font.pointSize: 12
            Layout.fillHeight: false
            Layout.fillWidth: true
        }

        TextField {
            id: portField
            placeholderText: qsTr("Port")
            text: "9988"
            font.pointSize: 12
            Layout.fillWidth: true
            validator: IntValidator { bottom: 1; top: 65535 }
        }

        TextField {
            id: usernameField
            font.pointSize: 12
            placeholderText: qsTr("Username")
            Layout.fillWidth: true
        }

        TextField {
            id: passwordField
            font.pointSize: 12
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
            Layout.fillWidth: true
        }

        Button {
            text: "Connect and Login"
            font.pointSize: 12
            Layout.fillWidth: true
            enabled: !apiClient.busy
            onClicked: {
                apiClient.connectToServer(hostField.text, parseInt(portField.text))
            }
        }

        Label {
            id: statusLabel
            text: apiClient.connected ? qsTr("Connected") : qsTr("Disconnected")
            font.pointSize: 12
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
    width: 1024
    height: 768
}
