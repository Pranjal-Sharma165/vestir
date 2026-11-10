import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Vestir

Pane {
    id: root
    padding: 0
    background: Item {}

    required property OutfitRecommendation model
    property bool decided: false

    signal notForMe
    signal wearToday

    // small rounded label used for weather / event info
    component InfoChip: Rectangle {
        property alias text: chipText.text
        implicitHeight: 30
        implicitWidth: chipText.implicitWidth + 24
        radius: height / 2
        color: "#FFFFFF"
        border.width: 1
        border.color: Theme.colours.background_c1

        Text {
            id: chipText
            anchors.centerIn: parent
            font: Theme.font.body
            color: Theme.colours.text
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 14

        // Title + piece count
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.model.qTitle
                font: Theme.font.title
                color: Theme.colours.text
            }
            Text {
                text: root.model.qPieceCount + (root.model.qPieceCount === 1 ? " piece" : " pieces")
                font: Theme.font.caption
                color: Theme.colours.alt_c2
            }
        }

        // Image card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 380
            radius: 22
            color: "#FFFFFF"

            Item {
                id: imageMask
                anchors.fill: imageArea
                visible: false
                layer.enabled: true
                Rectangle {
                    anchors.fill: parent
                    radius: 16
                }
            }

            Item {
                id: imageArea
                anchors.fill: parent
                anchors.margins: 8
                layer.enabled: true
                layer.effect: MultiEffect {
                    maskEnabled: true
                    maskSource: imageMask
                }

                ImagePlaceholder {}

                Row {
                    anchors.fill: parent
                    spacing: 2

                    Repeater {
                        model: root.model.qPieceList

                        Image {
                            required property var modelData
                            width: (parent.width - parent.spacing * (root.model.qPieceCount - 1))
                                   / Math.max(1, root.model.qPieceCount)
                            height: parent.height
                            source: modelData.qImageUrl
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }
                }
            }
        }

        // Piece list
        Text {
            Layout.fillWidth: true
            Layout.preferredWidth: 0 // fixes horizontal scrolling with long lists
            font: Theme.font.subtitle
            color: Theme.colours.text
            wrapMode: Text.WordWrap
            text: formatPieceListString(root.model.qPieceList)

            function formatPieceListString(items) {
                if (!items || items.length === 0) return "";
                const itemStrings = items.map(item => item.qName);
                if (items.length === 1) return itemStrings[0];
                if (items.length === 2) return itemStrings.join(" and ");
                return itemStrings.slice(0, -1).join(", ") + ", and " + itemStrings[items.length - 1];
            }
        }

        // Weather chips
        Loader {
            Layout.fillWidth: true
            active: !!root.model.qWeather
            visible: active
            sourceComponent: Flow {
                width: parent ? parent.width : 0
                spacing: 8

                InfoChip { text: `${root.model.qWeather.degrees_celcius}°C · ${root.model.qWeather.cloudiness}` }
                InfoChip { text: root.model.qWeather.city }
                InfoChip { text: `${root.model.qWeather.min_daily_temp_celcius}–${root.model.qWeather.max_daily_temp_celcius}°C` }
                InfoChip { text: `Rain ${Math.round(root.model.qWeather.rain_chance * 100)}%` }
            }
        }

        // Event chips
        Loader {
            Layout.fillWidth: true
            active: !!root.model.qEvent
            visible: active
            sourceComponent: Flow {
                width: parent ? parent.width : 0
                spacing: 8

                InfoChip { text: root.model.qEvent.qName }
                InfoChip { text: OutfitFormality.qDisplayName(root.model.qEvent.qFormality) }
            }
        }

        // Rating buttons
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 12
            visible: !root.decided

            StyledButton {
                Layout.fillWidth: true
                text: "Not for me"
                onClicked: root.notForMe()
            }
            StyledButtonDark {
                Layout.fillWidth: true
                text: "Wear today"
                onClicked: root.wearToday()
            }
        }

        // Confirmation banner
        Rectangle {
            Layout.fillWidth: true
            visible: root.decided
            implicitHeight: 52
            radius: 16
            color: Theme.colours.alt_c3

            Text {
                anchors.centerIn: parent
                text: "Great choice! Enjoy your outfit."
                font: Theme.font.bodyBold
                color: Theme.colours.alt_c1
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
