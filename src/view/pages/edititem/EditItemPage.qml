import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

// The page shown when editing an existing clothing item.
Page {
    id: root
    required property ClothingItem clothingItem
    required property StackView stack

    ColumnLayout {
        anchors.fill: parent

        Text {
            text: clothingItem.qName
            font: Theme.title
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.imagePath
            fillMode: Image.PreserveAspectFit
        }

        Button {
            text: "Change image"
            onClicked: root.stack.pop()
        }

        GridLayout {
            columns: 2
            Layout.fillWidth: true
            uniformCellWidths: true
            uniformCellHeights: true
            Text {
                text: `Category: ${ClothingCategory.displayName(clothingItem.category)}`
            }
        }

        Item {
            Layout.fillHeight: true
        }

        Button {
            text: "Add to closet"
            onClicked: {
                root.stack.pop(null)
            }
        }
    }
}