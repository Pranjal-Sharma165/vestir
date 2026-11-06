import QtQuick
import QtCore
import QtMultimedia
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

Page {
    id: root
    required property StackView stack

    CameraPermission { id: cameraPermission }

    Component.onCompleted: {
        if (cameraPermission.status === Qt.PermissionStatus.Undetermined)
            cameraPermission.request()
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
            }
            imageCapture: ImageCapture {
                onImageSaved: function (id, path) {
                    console.log("id:", id)
                    console.log("path:", path)
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
            }
            Button {
                id: shootButton
                text: "Take photo"
                onClicked: {
                    captureSession.imageCapture.captureToFile()
                }
            }

            MediaDevices {
                id: mediaDevices
            }

            Button {
                text: "Flip camera"
            }
        }
    }
}
