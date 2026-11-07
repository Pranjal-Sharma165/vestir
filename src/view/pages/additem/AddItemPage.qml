import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

// The page shown when a user is in the process of adding a new item.
Page {
    id: root
    required property ClothingItem clothingItem
    required property StackView stack
}
