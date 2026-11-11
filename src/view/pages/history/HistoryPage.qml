import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    id: root
    required property HistoryPageModel historyPageModel

    onVisibleChanged: if (visible) historyPageModel.reload()

    background: Rectangle {
        color: Theme.colours.background_c2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin
        spacing: 12

        Label {
            text: qsTr("History")
            font: Theme.font.title
            color: Theme.colours.text
            Layout.bottomMargin: 12
        }

        Label {
            visible: historyPageModel.entries.length === 0
            Layout.fillWidth: true
            Layout.topMargin: 20
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font: Theme.font.body
            color: Theme.colours.alt_c2
            text: qsTr("No outfits yet. Outfits you wear will show up here.")
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: historyPageModel.entries.length > 0
            clip: true
            spacing: 12
            bottomMargin: Theme.spacing.pageBottom
            model: historyPageModel.entries

            delegate: Rectangle {
                id: card
                required property var modelData
                width: list.width
                height: cardContent.implicitHeight + 24
                radius: 22
                color: "#FFFFFF"

                ColumnLayout {
                    id: cardContent
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            Layout.fillWidth: true
                            text: card.modelData.title
                            font: Theme.font.bodyBold
                            color: Theme.colours.text
                            elide: Text.ElideRight
                        }
                        Label {
                            text: Qt.formatDate(card.modelData.when, "MMM d")
                            font: Theme.font.caption
                            color: Theme.colours.alt_c2
                        }
                    }

                    Row {
                        spacing: 8
                        Repeater {
                            model: card.modelData.pieces
                            delegate: Image {
                                autoTransform: true
                                required property var modelData
                                width: 72
                                height: 72
                                source: modelData.imageUrl
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font: Theme.font.caption
                        color: Theme.colours.text
                        text: card.modelData.pieces.map(p => p.name).join(", ")
                    }
                }
            }
        }
    }
}
