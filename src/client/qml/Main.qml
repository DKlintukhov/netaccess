import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NetAccess 1.0

ApplicationWindow {
    id: root
    visible: true
    width: 1024
    height: 768
    title: "netaccess " + netaccessVersion
    background: Image {
        source: "qrc:/qml/images/background.jpeg"
        fillMode: Image.PreserveAspectCrop
    }

    ApiClient {
        id: apiClient
        onResponseReceived: function(reqId, response) {
            pageLoader.handleResponse(reqId, response)
        }
        onConnectionError: function(msg) {
            errorLabel.text = msg
            errorDialog.open()
        }
    }

    SessionState {
        id: sessionState
    }

    Dialog {
        id: errorDialog
        modal: true
        title: qsTr("Error")
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        Label {
            id: errorLabel
            text: ""
            wrapMode: Text.Wrap
        }
    }

    Loader {
        id: pageLoader
        anchors.fill: parent
        sourceComponent: sessionState.authenticated ? mainPage : loginPage

        property int pendingReqId: 0

        function handleResponse(reqId, response) {
            if (pageLoader.item && pageLoader.item.handleResponse) {
                pageLoader.item.handleResponse(reqId, response)
            }
        }
    }

    Component {
        id: loginPage
        LoginPage {
            client: apiClient
            session: sessionState
        }
    }

    Component {
        id: mainPage
        MainPage {
            client: apiClient
            session: sessionState
        }
    }
}
