import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import Vestir
import Vestir.DesignTokens as Tokens

Pane {
    id: root
    padding: 0

    required property OutfitRecommendation model

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Text {
            wrapMode: Text.WordWrap
            text: model.title
            font: Theme.title
        }

        Pane {
            Layout.fillWidth: true
            height: 400
            padding: 0

            RowLayout {
                anchors.fill: parent
                spacing: 4
                Repeater {
                    model: root.model.images
                    Image {
                        required property url modelData
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        source: modelData
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true
                        sourceSize.height: 600
                    }
                }
            }
        }

        Text {
            visible: text.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.model.message
        }

        Text {
            font.pixelSize: 13
            text: model.pieceCount + " pieces"
        }

        Text {
            font.pixelSize: 16
            Layout.fillWidth: true
            text: model.pieceList
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
                    RowLayout {
                        Text {
                            text: `${model.weather.degrees_celcius}°C`
                        }
                        Text {
                            text: ` | `
                        }
                        Text {
                            text: model.weather.cloudiness
                        }
                    }

                    RowLayout {
                        Text {
                            text: model.weather.city
                        }
                        Text {
                            text: " | "
                        }
                        Text {
                            text: `${model.weather.min_daily_temp_celcius}-${model.weather.max_daily_temp_celcius}°C`
                        }
                        Text {
                            text: " | "
                        }
                        Text {
                            text: `Rain ${model.weather.rain_chance * 100}%`
                        }
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
                    text: model.eventTitle
                }
                Text {
                    text: " | "
                }
                Text {
                    text: model.outfitFormality
                }
            }
        }

        RowLayout {
            id: rating_buttons_layout
            Button {
                Layout.fillWidth: true
                text: "Not for me"
                enabled: !root.model.loading
                onClicked: root.model.notForMe()
            }
            Button {
                Layout.fillWidth: true
                text: "Wear today"
                enabled: !root.model.loading
                onClicked: root.model.wearToday()
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
