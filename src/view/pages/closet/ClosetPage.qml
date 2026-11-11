import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    background: Rectangle {
        color: Theme.colours.background_c2
    }

    required property Closet closet
    required property StackView stack
    property int selectedCategory: -1

    function iconFor(name) {
        const n = name.toLowerCase()
        if (n.includes("outer") || n.includes("coat") || n.includes("jacket")) return "outerwear"
        if (n.includes("dress")) return "dress"
        if (n.includes("bottom") || n.includes("pant")) return "bottom"
        return "top"
    }

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
        spacing: 20
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "My Wardrobe"
                font: Theme.font.title
                color: Theme.colours.text
            }
            Item {
                Layout.fillWidth: true
            }

            StyledButtonDark {
                text: "Upload"
                onClicked: stack.push(cameraPage)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 5
            spacing: 10

            Repeater {
                model: ClothingCategory.qGetList()

                delegate: CategoryBlock {
                    required property var modelData

                    Layout.fillWidth: true
                    Layout.preferredWidth: 0   // keeps all four blocks equal width
                    iconName: root.iconFor(ClothingCategory.qCategoryDisplayName(modelData))
                    label: ClothingCategory.qCategoryDisplayName(modelData)
                    // Reading closet.items makes the binding re-run when the closet changes.
                    itemCount: closet.items ? closet.count(modelData) : 0
                    selected: root.selectedCategory === modelData
                    onClicked: root.selectedCategory = selected ? -1 : modelData
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
