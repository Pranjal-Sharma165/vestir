import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "Closet"
                font: Theme.title
            }
            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Add clothing"
            }
        }
        TextField {
            placeholderText: "Find an item"
        }
        Flow {
            Layout.fillWidth: true
            spacing: 10
            Button {
                checkable: true
                checked: true
                text: "All ()"
            }
            Button {
                checkable: true
                checked: false
                text: "Tops ()"
            }
            Button {
                checkable: true
                checked: false
                text: "Bottoms ()"
            }
            Button {
                checkable: true
                checked: false
                text: "Outerwear ()"
            }
            Button {
                checkable: true
                checked: false
                text: "Shoes ()"
            }
            Button {
                checkable: true
                checked: false
                text: "Accessories ()"
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Button {
                text: "Filter"
            }
            Item {
                Layout.fillWidth: true
            }
            Button {
                text: "Recently added"
            }
        }

        ClothingItem {
            id: item
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            contentWidth: availableWidth
            GridLayout {
                width: scrollView.availableWidth
                property int desiredItemWidth: 150
                columns: Math.max(2, Math.floor(width / (desiredItemWidth + columnSpacing)))
                // made-up data for now until we get data from the database later.
                Repeater {
                    model: 16
                    ClosetCard {
                        clothingItem: item
                    }
                }
            }
        }
    }
}
