
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

Pane {
    id: root

    required property StackView stack
    required property ClothingItem clothingItem

    padding: 10

    background: Rectangle {
        color: "#FFFFFF"
        radius: 14
    }

    TapHandler {
        onTapped: root.stack.push(editItemPage)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            clip: true

            ImagePlaceholder {}

            Image {
                autoTransform: true
                anchors.fill: parent
                source: root.clothingItem.qImageUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.clothingItem.qName
            font: Qt.font({
                family: Theme.font.body.family,
                pixelSize: Theme.font.body.pixelSize,
                bold: true
            })
            color: Theme.colours.text
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: ClothingCategory.qCategoryDisplayName(
                      root.clothingItem.category)
            font: Theme.font.caption
            color: Theme.colours.alt_c2
        }
    }

    Component {
        id: editItemPage

        EditItemPage {
            stack: root.stack
            clothingItem: root.clothingItem
        }
    }
}
