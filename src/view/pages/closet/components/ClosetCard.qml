import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

Pane {
    id: root
    required property StackView stack
    required property ClothingItem clothingItem
    padding: 0

    TapHandler {
        onTapped: stack.push(editItemPage)
    }

    ColumnLayout {
        anchors.fill: parent
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            ImagePlaceholder {}
        }
        Text {
            text: clothingItem.qName
        }
        Text {
            text: ClothingCategory.qCategoryDisplayName(clothingItem.category)
        }

        Component {
            id: editItemPage
            EditItemPage {
                stack: root.stack
                clothingItem: root.clothingItem
            }
        }
    }
}
