import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property Closet closet
    required property StackView stack
    property int selectedCategory: -1

    Component {
        id: cameraPage
        CameraPage {
            stack: root.stack
        }
    }

    // filtering buttons
    ButtonGroup { id: categoryGroup }

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
                onClicked: stack.push(cameraPage)
            }
        }

        TextField {
            placeholderText: "Find an item"
        }

        Flow {
            Layout.fillWidth: true
            spacing: 10
            Button {
                ButtonGroup.group: categoryGroup
                checkable: true
                checked: true
                text: `All (${closet.count()})`
                onClicked: root.selectedCategory = -1
            }
            Repeater {
                model: ClothingCategory.qGetList()

                delegate: Button {
                    ButtonGroup.group: categoryGroup
                    checkable: true
                    checked: false
                    text: `${ClothingCategory.qCategoryDisplayName(modelData)} (${closet.count(modelData)})`
                    onClicked: root.selectedCategory = modelData
                }
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
                uniformCellWidths: true
                uniformCellHeights: true
                property int desiredItemWidth: 150
                columns: Math.max(2, Math.floor(width / (desiredItemWidth + columnSpacing)))
                Repeater {
                    model: closet.items
                    delegate: ClosetCard {
                        required property ClothingItem modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clothingItem: modelData
                        stack: root.stack
                        visible: root.selectedCategory < 0
                                 || modelData.category === root.selectedCategory
                    }
                }
            }
        }
    }
}
