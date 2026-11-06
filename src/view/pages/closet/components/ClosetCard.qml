import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Vestir

ColumnLayout {
    required property ClothingItem clothingItem
    Pane {
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        padding: 0
        ImagePlaceholder {}
    }
    Text {
        text: clothingItem.nameQString
    }
    Text {
        text: ClothingCategory.displayName(clothingItem.category)
    }
}
