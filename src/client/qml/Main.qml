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
    Image {
        id: backgroundImage
        anchors.fill: parent
        source: "images/background.jpeg"

        // Controls how the image scales to fit the window size
        fillMode: Image.PreserveAspectCrop

        // Places the image behind other UI components
        z: -1
    }

    // Shared components.
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

    // Error dialog.
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

    // Main content.
    Loader {
        id: pageLoader
        anchors.fill: parent
        sourceComponent: sessionState.authenticated ? mainPage : loginPage

        property int pendingReqId: 0

        function handleResponse(reqId, response) {
            // Delegate to the current page.
            if (pageLoader.item && pageLoader.item.handleResponse) {
                pageLoader.item.handleResponse(reqId, response)
            }
        }
    }

    Component {
        id: loginPage
        LoginPage {
            apiClient: apiClient
            sessionState: sessionState
        }
    }

    Component {
        id: mainPage
        MainPage {
            apiClient: apiClient
            sessionState: sessionState
        }
    }
}
