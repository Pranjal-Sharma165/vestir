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
        Image {
            anchors.fill: parent
            source: clothingItem.imageUrl
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            sourceSize.height: 400
        }
    }
    Text {
        text: clothingItem.nameQString
    }
    Text {
        text: ClothingCategory.displayName(clothingItem.category)
    }
}
