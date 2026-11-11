import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

// The page shown when editing an existing clothing item.
Page {
    id: root
    required property ClothingItem clothingItem
    required property StackView stack

    // The user's corrections; start from what the classifier said.
    property var category: clothingItem.category
    property var subCategory: clothingItem.subCategory
    property var colour: clothingItem.colour
    property string error: ""

    background: Rectangle {
        color: Theme.colours.background_c2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin
        spacing: 14

        Text {
            text: qsTr("Edit item")
            font: Theme.font.title
            color: Theme.colours.text
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: root.clothingItem.qImageUrl
            fillMode: Image.PreserveAspectFit
        }

        ItemLabelPicker {
            Layout.fillWidth: true
            category: root.category
            subCategory: root.subCategory
            colour: root.colour
            onEdited: (cat, sub, col) => {
                root.category = cat
                root.subCategory = sub
                root.colour = col
            }
        }

        Text {
            visible: root.error !== ""
            Layout.fillWidth: true
            text: root.error
            color: "#B00020"
            font: Theme.font.caption
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledButton {
                Layout.fillWidth: true
                text: qsTr("Back")
                onClicked: root.stack.pop(null)
            }
            StyledButtonDark {
                Layout.fillWidth: true
                text: qsTr("Save changes")
                onClicked: {
                    const message = appCloset.updateItem(
                        root.clothingItem, root.category, root.subCategory,
                        root.colour, root.clothingItem.formality)
                    if (message === "")
                        root.stack.pop(null)
                    else
                        root.error = message
                }
            }
        }
    }
}
