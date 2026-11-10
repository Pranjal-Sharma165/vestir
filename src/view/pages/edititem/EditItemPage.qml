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
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin

        Text {
            text: clothingItem.qName
            font: Theme.font.title
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.clothingItem.qImageUrl
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
                text: `Category: ${ClothingCategory.qCategoryDisplayName(clothingItem.category)}`
            }
            Text {
                text: `Sub-category: ${ClothingCategory.qSubCategoryDisplayName(clothingItem.subCategory)}`
            }
            Text {
                text: `Colour: ${ClothingColour.qDisplayName(clothingItem.colour)}`
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Button {
                text: "Back"
                onClicked: {
                    root.stack.pop(null)
                }
            }
            Button {
                text: "Save changes"
                onClicked: {
                    root.stack.pop(null)
                }
            }
        }
    }
}
