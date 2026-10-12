import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

ColumnLayout {
    required property ClothingItem clothingItem
    Pane {
        Layout.fillWidth: true
        Layout.preferredHeight: 200
        padding: 0
        ImagePlaceholder {}
    }
    Text {
        text: clothingItem.name
    }
    Text {
        text: clothingItem.category
    }
}
