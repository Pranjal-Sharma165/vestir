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

    readonly property string errorText: {
        if (permissionDenied)
            return "Camera access was denied."
        if (noDevice)
            return "No camera was found on this device."
        if (cameraErrorMessage !== "")
            return "The camera could not be used: " + cameraErrorMessage
        return ""
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

        RowLayout {
            Layout.fillWidth: true

            Button {
                text: "Back"
                onClicked: stack.pop()
            }

            Button {
                text: "Toggle flash"
            }
        }

        CaptureSession {
            id: captureSession
            videoOutput: output

            camera: Camera {
                active: root.StackView.status === StackView.Active
                        && cameraPermission.status === Qt.PermissionStatus.Granted
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

        VideoOutput {
            Layout.fillWidth: true
            Layout.preferredHeight: 400
            id: output
        }

        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "Gallery"
                onClicked: galleryDialog.open()
            }
            Button {
                id: shootButton
                text: "Take photo"
                onClicked: captureSession.imageCapture.captureToFile(CameraHelper.newPhotoPath())
            }

            Button {
                text: "Flip camera"
            }
        }
    }
}
