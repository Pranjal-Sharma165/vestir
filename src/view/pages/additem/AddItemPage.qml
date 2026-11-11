import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

// The page shown when a user is in the process of adding a new item.
Page {
    id: root
    required property url imagePath
    required property StackView stack

    readonly property bool loading: imageRecognitionHelper.state === ImageRecognitionHelper.Loading
    readonly property bool failed: imageRecognitionHelper.state === ImageRecognitionHelper.Failed
    readonly property bool ready: imageRecognitionHelper.state === ImageRecognitionHelper.Ready

    // null until imageRecognitionHelper is Ready
    readonly property ClothingItem clothingItem: imageRecognitionHelper.item

    background: Rectangle {
        color: Theme.colours.background_c2
    }

    ImageRecognitionHelper {
        id: imageRecognitionHelper
    }

    Component.onCompleted: imageRecognitionHelper.start(root.imagePath)

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin
        spacing: 14

        Text {
            text: root.loading ? qsTr("Analysing photo...") : qsTr("New item")
            font: Theme.font.title
            color: Theme.colours.text
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.imagePath
            fillMode: Image.PreserveAspectFit
        }

        BusyIndicator {
            visible: root.loading
            running: root.loading
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            visible: root.failed
            Layout.fillWidth: true
            text: qsTr("Couldn't classify this photo. Check that the ML server is running and try again.\n(%1)")
                      .arg(imageRecognitionHelper.error)
            color: "#B00020"
            font: Theme.font.caption
            wrapMode: Text.WordWrap
        }

        Text {
            visible: root.ready
            Layout.fillWidth: true
            text: qsTr("Not right? Change it below.")
            font: Theme.font.caption
            color: Theme.colours.alt_c2
        }

        ItemLabelPicker {
            visible: root.ready
            Layout.fillWidth: true
            category: root.clothingItem ? root.clothingItem.category : undefined
            subCategory: root.clothingItem ? root.clothingItem.subCategory : undefined
            colour: root.clothingItem ? root.clothingItem.colour : undefined
            onEdited: (cat, sub, col) => imageRecognitionHelper.relabel(cat, sub, col)
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledButton {
                Layout.fillWidth: true
                text: qsTr("Back")
                onClicked: {
                    appCloset.discardPhoto(root.imagePath)
                    root.stack.pop()
                }
            }
            StyledButtonDark {
                Layout.fillWidth: true
                text: qsTr("Add to closet")
                enabled: root.ready
                onClicked: {
                    appCloset.addItem(root.clothingItem)
                    root.stack.pop(null)
                }
            }
        }
    }
}
