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
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "My Wardrobe"
                font: Theme.font.title
            }
            Item {
                Layout.fillWidth: true
            }

            StyledButtonDark {
                text: "Add clothing"
                onClicked: stack.push(cameraPage)
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 10
            StyledButton {
                ButtonGroup.group: categoryGroup
                checkable: true
                checked: true
                text: `All (${closet.count()})`
                onClicked: root.selectedCategory = -1
            }
            Repeater {
                model: ClothingCategory.qGetList()

                delegate: StyledButton {
                    ButtonGroup.group: categoryGroup
                    checkable: true
                    checked: false
                    text: `${ClothingCategory.qCategoryDisplayName(modelData)} (${closet.count(modelData)})`
                    onClicked: root.selectedCategory = modelData
                }
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
