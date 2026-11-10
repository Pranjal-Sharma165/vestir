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

    ImageRecognitionHelper {
        id: imageRecognitionHelper
    }

    Component.onCompleted: imageRecognitionHelper.start(root.imagePath)

    ColumnLayout {
        anchors.fill: parent

        Text {
            text: root.clothingItem ? root.clothingItem.qName : "Loading"
            font: Theme.font.title
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.imagePath
            fillMode: Image.PreserveAspectFit
        }

        Button {
            text: "Change image"
            onClicked: {
                appCloset.discardPhoto(root.imagePath)
                root.stack.pop()
            }
        }

        Text {
            visible: root.failed
            text: "Classification failed: " + imageRecognitionHelper.error
            color: "red"
            wrapMode: Text.WordWrap
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            uniformCellWidths: true
            uniformCellHeights: true

            Text {
                text: `Category: ${root.clothingItem ? ClothingCategory.qCategoryDisplayName(root.clothingItem.category) : "Loading"}`
            }
            Text {
                text: `Sub-category: ${root.clothingItem ? ClothingCategory.qSubCategoryDisplayName(root.clothingItem.subCategory) : "Loading"}`
            }
            Text {
                text: `Colour: ${root.clothingItem ? ClothingColour.qDisplayName(root.clothingItem.colour) : "Loading"}`
            }
        }

        RowLayout {
            Button {
                text: "Back"
                onClicked: {
                    appCloset.discardPhoto(root.imagePath)
                    root.stack.pop()
                }
            }
            Button {
                text: "Add"
                enabled: root.ready
                onClicked: {
                    appCloset.addItem(root.clothingItem)
                    root.stack.pop(null)
                }
            }
        }
    }
}
