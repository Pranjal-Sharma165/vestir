import QtQuick
import QtCore
import QtQuick.Dialogs
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

Page {
    id: root
    required property StackView stack

    property string cameraErrorMessage: ""
    property bool flashOn: false

    background: Rectangle {
        color: Theme.colours.background_c2
    }

    CameraPermission { id: cameraPermission }
    MediaDevices { id: mediaDevices }

    readonly property bool permissionDenied:
        cameraPermission.status === Qt.PermissionStatus.Denied
    readonly property bool noDevice:
        mediaDevices.videoInputs.length === 0
    readonly property bool cameraUsable:
        cameraPermission.status === Qt.PermissionStatus.Granted
        && !noDevice
        && cameraErrorMessage === ""

    // Only offer flash / flip when the device can actually do them
    readonly property bool canFlip: mediaDevices.videoInputs.length > 1
    readonly property bool canFlash: {
        camera.cameraDevice
        camera.active
        return camera.isFlashModeSupported(Camera.FlashOn)
    }

    readonly property string errorText: {
        if (permissionDenied)
            return "Camera access was denied."
        if (noDevice)
            return "No camera was found on this device."
        if (cameraErrorMessage !== "")
            return "The camera could not be used: " + cameraErrorMessage
        return ""
    }

    function flipCamera() {
        const inputs = mediaDevices.videoInputs
        if (inputs.length < 2)
            return
        let current = 0
        for (let i = 0; i < inputs.length; ++i) {
            if (inputs[i].id === camera.cameraDevice.id)
                current = i
        }
        root.flashOn = false
        camera.cameraDevice = inputs[(current + 1) % inputs.length]
    }

    Component.onCompleted: {
        CameraHelper.ensureDir()
        if (cameraPermission.status === Qt.PermissionStatus.Undetermined)
            cameraPermission.request()
    }

    Component {
        id: addItemPage
        AddItemPage {
            stack: root.stack
        }
    }


    FileDialog {
        id: galleryDialog
        title: "Choose a photo"
        nameFilters: ["Images (*.jpg *.jpeg *.png *.webp *.bmp)"]
        onAccepted: {
            const copy = CameraHelper.importPhoto(selectedFile)
            if (copy.toString() !== "")
                root.stack.push(addItemPage, { imagePath: copy })
            else
                console.warn("Could not import the chosen photo")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin
        spacing: 16

        RowLayout {
            Layout.fillWidth: true

            StyledButton {
                text: qsTr("Back")
                onClicked: root.stack.pop()
            }

            Item { Layout.fillWidth: true }

            StyledButton {
                visible: root.canFlash
                text: root.flashOn ? qsTr("Flash on") : qsTr("Flash off")
                checkable: true
                checked: root.flashOn
                onClicked: root.flashOn = !root.flashOn
            }
        }

        CaptureSession {
            id: captureSession
            videoOutput: output

            camera: Camera {
                id: camera
                active: root.StackView.status === StackView.Active
                        && cameraPermission.status === Qt.PermissionStatus.Granted
                flashMode: root.flashOn ? Camera.FlashOn : Camera.FlashOff
                onErrorOccurred: function (error, message) {
                    root.cameraErrorMessage = message
                }
            }
            imageCapture: ImageCapture {
                fileFormat: ImageCapture.JPEG
                quality: ImageCapture.HighQuality
                onImageSaved: function (id, path) {
                    Qt.callLater(function () {
                        root.stack.push(addItemPage, { imagePath: CameraHelper.toUrl(path) })
                    })
                    console.log("path:", path)
                }
                onErrorOccurred: function (id, error, message) {
                    console.warn("Capture failed:", message)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 22
            color: "#1B1B1B"
            clip: true

            VideoOutput {
                id: output
                anchors.fill: parent
                visible: root.cameraUsable
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 48
                visible: !root.cameraUsable && root.errorText !== ""
                text: root.errorText
                color: "#FFFFFF"
                font: Theme.font.body
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                text: qsTr("Gallery")
                onClicked: galleryDialog.open()
            }

            // Round shutter button
            Button {
                id: shootButton
                Layout.preferredWidth: 72
                Layout.preferredHeight: 72
                enabled: root.cameraUsable
                onClicked: captureSession.imageCapture.captureToFile(CameraHelper.newPhotoPath())
                background: Rectangle {
                    radius: width / 2
                    color: "#FFFFFF"
                    border.width: 4
                    border.color: Theme.colours.alt_c1
                    opacity: shootButton.enabled ? 1 : 0.4

                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - 16
                        height: width
                        radius: width / 2
                        color: shootButton.pressed ? Theme.colours.alt_c1_pressed
                                                   : Theme.colours.alt_c1
                    }
                }
            }

            StyledButton {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                enabled: root.canFlip
                text: qsTr("Flip camera")
                onClicked: root.flipCamera()
            }
        }
    }
}
