import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material

import org.streetpea.chiaking

Rectangle {
    id: view
    property bool allowClose: false
    property bool cancelling: false
    property bool textVisible: true
    property bool registOnly: false
    property bool isForbiddenError: false
    property list<Item> restoreFocusItems
    color: "black"

    StackView.onActivated: {
        infoLabel.visible = false
        isForbiddenError = false
        textVisible = true
        cancelling = false
    }

    function stop() {
        if (!allowClose)
            return;
        allowClose = false;
        cancelling = true;
        infoLabel.text = qsTr("Cancelling connection with console...");
        Chiaki.psnCancel(false);
    }

    function grabInput(item) {
        Chiaki.window.grabInput();
        restoreFocusItems.push(Window.window.activeFocusItem);
        if (item)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    function releaseInput() {
        Chiaki.window.releaseInput();
        let item = restoreFocusItems.pop();
        if (item && item.visible)
            item.forceActiveFocus(Qt.TabFocusReason);
    }

    Keys.onEscapePressed: {
        if (isForbiddenError)
            root.showMainView();
        else
            view.stop();
    }

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: {
            if (isForbiddenError)
                root.showMainView();
            else
                view.stop();
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: view.allowClose && !isForbiddenError
        acceptedButtons: Qt.RightButton
        onClicked: view.stop()
    }

    Label {
        id: infoLabel
        anchors.centerIn: parent
        opacity: (textVisible && !isForbiddenError) ? 1.0: 0.0
        visible: opacity > 0
        text: qsTr("Establishing connection with console...")
        Behavior on opacity { NumberAnimation { duration: 250 } }
    }

    Item {
        anchors {
            top: parent.verticalCenter
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        BusyIndicator {
            id: spinner
            anchors.centerIn: parent
            width: 70
            height: width
            visible: !isForbiddenError
            running: !isForbiddenError
        }

        Label {
            id: closeMessageLabel
            anchors {
                top: spinner.bottom
                horizontalCenter: spinner.horizontalCenter
                topMargin: 30
            }
            opacity: (textVisible && !cancelling && !isForbiddenError) ? 1.0: 0.0
            visible: opacity > 0
            text: {
                var typeString = registOnly ? qsTr("automatic registration") : qsTr("remote connection")
                qsTr("Press %1 to cancel %2").arg(Chiaki.controllers.length ? (root.controllerButton("circle").includes("deck") ? "B" : "Circle") : "escape or right-click").arg(typeString)
            }
        }

        Label {
            id: errorTitleLabel
            anchors {
                bottom: spinner.top
                horizontalCenter: spinner.horizontalCenter
            }
            font.pixelSize: 24
            visible: text && !isForbiddenError
            onVisibleChanged: {
                if (visible) {
                    textVisible = false
                    view.allowClose = false
                    view.grabInput(errorTitleLabel)
                }
            }
            Keys.onReturnPressed: root.showMainView()
            Keys.onEscapePressed: root.showMainView()
        }

        Label {
            id: errorTextLabel
            anchors {
                top: errorTitleLabel.bottom
                horizontalCenter: errorTitleLabel.horizontalCenter
                topMargin: 10
            }
            visible: !isForbiddenError
        }
    }

    ColumnLayout {
        id: forbiddenErrorBox
        visible: isForbiddenError
        anchors.centerIn: parent
        width: Math.min(parent.width - 60, 560)
        spacing: 24

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 20
            font.bold: true
            color: "#FF6B6B"
            text: qsTr("Conexión remota rechazada")
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 15
            color: "#E0E0E0"
            text: qsTr("Sony ha rechazado la conexión remota. Prueba más tarde o registra la consola por PIN (más rápido en casa)")
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 16

            Button {
                text: qsTr("Registrar por PIN (en casa)")
                highlighted: true
                Material.accent: "#6C5CE7"
                onClicked: {
                    root.showMainView()
                    let hostToUse = Chiaki.sessionHost() && Chiaki.sessionHost().length > 0 ? Chiaki.sessionHost() : "255.255.255.255"
                    root.showRegistDialog(hostToUse, Chiaki.sessionIsPs5())
                }
            }

            Button {
                text: qsTr("Volver")
                flat: true
                onClicked: {
                    root.showMainView()
                }
            }
        }
    }

    Timer {
        id: closeTimer
        interval: 1500
        running: true
        onTriggered: {
            view.allowClose = true
            infoLabel.visible = infoLabel.opacity
        }
    }

    Timer {
        id: failTimer
        interval: 2000
        running: false
        onTriggered: root.showMainView()
    }

    Dialog {
        id: sessionPinDialog
        parent: Overlay.overlay
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        title: qsTr("Console Login PIN")
        modal: true
        closePolicy: Popup.NoAutoClose
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAboutToShow: {
            standardButton(Dialog.Ok).enabled = Qt.binding(function() {
                return pinField.acceptableInput;
            });
            textVisible = false
            view.grabInput(pinField);
        }
        onClosed: {
            textVisible = true
            view.releaseInput()
        }
        onAccepted: Chiaki.enterPin(pinField.text)
        onRejected: Chiaki.stopSession(false)
        Material.roundedScale: Material.MediumScale

        TextField {
            id: pinField
            echoMode: Chiaki.settings.streamerMode ? TextInput.Password : TextInput.Normal
            implicitWidth: 200
            validator: RegularExpressionValidator { regularExpression: /[0-9]{4}/ }
            Keys.onReturnPressed: {
                if(sessionPinDialog.standardButton(Dialog.Ok).enabled)
                    sessionPinDialog.standardButton(Dialog.Ok).clicked()
            }
        }
    }

    Connections {
        target: Chiaki

        function onConnectStateChanged() 
        {
            switch(Chiaki.connectState)
            {
                case Chiaki.PsnConnectState.LinkingConsole:
                    infoLabel.text = registOnly ? qsTr("Registering PlayStation console with Pylux ...") : qsTr("Linking Pylux with PlayStation console ...")
                    view.allowClose = false
                    break
                case Chiaki.PsnConnectState.RegisteringConsole:
                    view.registOnly = true;
                    break
                case Chiaki.PsnConnectState.RegistrationFinished:
                    infoLabel.text = qsTr("Successfully registered console")
                    failTimer.restart()
                    break
                case Chiaki.PsnConnectState.DataConnectionStart:
                    infoLabel.text = qsTr("Console Linked ... Establishing data connection with console...")
                    view.allowClose = true
                    break
                case Chiaki.PsnConnectState.DataConnectionFinished:
                    view.allowClose = false
                    break
                case Chiaki.PsnConnectState.ConnectFailed:
                    if(!cancelling)
                        infoLabel.text = qsTr("Connection failed closing...")
                    failTimer.running = true
                    break
                case Chiaki.PsnConnectState.ConnectFailedStart:
                    if(!cancelling)
                        infoLabel.text = qsTr("Couldn't establish connection with PlayStation. Please try again...")
                    failTimer.running = true
                    break
                case Chiaki.PsnConnectState.ConnectFailedConsoleUnreachable:
                    if(!cancelling)
                        infoLabel.text = qsTr("Couldn't contact PlayStation over established connection, likely unsupported network type")
                    failTimer.running = true
                    break
                case Chiaki.PsnConnectState.ConnectFailedForbidden:
                    isForbiddenError = true
                    textVisible = false
                    view.allowClose = true
                    failTimer.stop()
                    break
                case Chiaki.PsnConnectState.WaitingForInternet:
                    infoLabel.text = qsTr("Establishing Internet Connection...")
                    break
            }
        }

        function onSessionChanged() 
        {
            if (!Chiaki.session) {
                if (isForbiddenError)
                    return;
                if (errorTitleLabel.text)
                    failTimer.start();
                else if(!failTimer.running)
                    root.showMainView();
            }
        }

        function onSessionError(title, text)
        {
            errorTitleLabel.text = title;
            errorTextLabel.text = text;
            closeTimer.start();
        }

        function onSessionPinDialogRequested() 
        {
            if (sessionPinDialog.opened)
                return;
            sessionPinDialog.open();
        }
    }
}
