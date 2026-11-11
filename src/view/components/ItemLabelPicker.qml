import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Vestir

// Three drop-downs (category, type, colour) to correct the classifier's labels.
// Emits edited() with the full new set whenever the user changes one.
ColumnLayout {
    id: root

    property var category
    property var subCategory
    property var colour

    signal edited(var category, var subCategory, var colour)

    readonly property var categories: ClothingCategory.qGetList()
    readonly property var subCategories: ClothingCategory.qGetList(root.category)
    readonly property var colours: ClothingColour.qGetList()

    spacing: 10

    component Field: ColumnLayout {
        id: field
        property alias label: caption.text
        property alias model: box.model
        property alias currentIndex: box.currentIndex
        property var displayName: function (v) { return "" }
        signal picked(int index)

        Layout.fillWidth: true
        spacing: 4

        Label {
            id: caption
            font: Theme.font.caption
            color: Theme.colours.alt_c2
        }

        ComboBox {
            id: box
            Layout.fillWidth: true
            implicitHeight: 44
            font: Theme.font.body
            displayText: currentIndex >= 0 ? field.displayName(model[currentIndex]) : ""
            onActivated: index => field.picked(index)

            delegate: ItemDelegate {
                required property int index
                width: box.width
                text: field.displayName(box.model[index])
                font: Theme.font.body
                highlighted: box.highlightedIndex === index
            }
            contentItem: Text {
                leftPadding: 14
                text: box.displayText
                font: box.font
                color: Theme.colours.text
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            background: Rectangle {
                radius: 22
                color: "#FFFFFF"
                border.width: 1.5
                border.color: box.pressed ? Theme.colours.alt_c1 : Theme.colours.background_c1
            }
        }
    }

    Field {
        label: qsTr("Category")
        model: root.categories
        currentIndex: root.categories.indexOf(root.category)
        displayName: v => ClothingCategory.qCategoryDisplayName(v)
        onPicked: index => {
            const cat = root.categories[index]
            const subs = ClothingCategory.qGetList(cat)
            root.edited(cat, subs[0], root.colour)
        }
    }

    Field {
        label: qsTr("Type")
        model: root.subCategories
        currentIndex: root.subCategories.indexOf(root.subCategory)
        displayName: v => ClothingCategory.qSubCategoryDisplayName(v)
        onPicked: index => root.edited(root.category, root.subCategories[index], root.colour)
    }

    Field {
        label: qsTr("Colour")
        model: root.colours
        currentIndex: root.colours.indexOf(root.colour)
        displayName: v => ClothingColour.qDisplayName(v)
        onPicked: index => root.edited(root.category, root.subCategory, root.colours[index])
    }
}
