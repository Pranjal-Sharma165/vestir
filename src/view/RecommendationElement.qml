import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Vestir
import Vestir.DesignTokens as Tokens

Pane {
    id: root

    required property Recommendation model

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            height: 40
            wrapMode: Text.WordWrap
            text: "Your outfit for the day"
            font.pixelSize: 28
        }

        Pane {
            Layout.fillWidth: true
            height: 400
            padding: 0

            ImagePlaceholder {}
        }

        Text {
            font.pixelSize: 13
            text: "5 pieces"
        }

        Text {
            font.pixelSize: 16
            Layout.fillWidth: true
            text: "<Shirt 1>, <Pants 2>, <Belt 3>, <Coat 4>, and <Shoes 5>"
            wrapMode: Text.WordWrap
        }

        Rectangle {
            id: weather_block
            height: weather_block_layout.height
            RowLayout {
                id: weather_block_layout
                Pane {
                    padding: 0
                    id: weather_icon_placeholder
                    height: 32
                    width: 32
                    ImagePlaceholder {}
                }
                ColumnLayout {
                    Text {
                        text: "16 degrees | Partly cloudy"
                    }
                    Text {
                        text: "Vancouver | 13-18 degrees | Rain 10%"
                    }
                }
            }
        }

        Rectangle {
            id: event_block
            height: event_block_layout.height
            RowLayout {
                id: event_block_layout
                Pane {
                    padding: 0
                    id: event_icon_placeholder
                    height: 32
                    width: 32
                    ImagePlaceholder {}
                }
                Text {
                    text: "<Event name>"
                }
                Text {
                    text: " | "
                }
                Text {
                    text: "<Outfit style>"
                }
            }
        }

        RowLayout {
            id: rating_buttons_layout
            Button {
                Layout.fillWidth: true
                text: "Not for me"
            }
            Button {
                Layout.fillWidth: true
                text: "Wear today"
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
