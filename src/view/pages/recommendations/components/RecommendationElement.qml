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
    property bool decided: false

    signal notForMe
    signal wearToday

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        // Title
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: model.qTitle
            font: Theme.font.title
        }

        // Image(s)
        Pane {
            Layout.fillWidth: true
            height: 400
            padding: 0

            ImagePlaceholder {}

            Row {
                anchors.fill: parent
                spacing: 2

                Repeater {
                    model: root.model.qPieceList

                    Image {
                        required property var modelData
                        width: (parent.width - parent.spacing * (root.model.qPieceCount - 1)) / Math.max(1, root.model.qPieceCount)
                        height: parent.height
                        source: modelData.qImageUrl
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        clip: true
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            font.pixelSize: 13
            text: model.qPieceCount + " pieces"
        }

        // The list of clothing items
        Text {
            Layout.fillWidth: true
            Layout.preferredWidth: 0 // fixes horizontal scrolling with long lists
            font.pixelSize: 16
            text: formatPieceListString(model.qPieceList)
            wrapMode: Text.WordWrap

            /*
             * Format a list of ClothingItems as a string containing their names.
             */
            function formatPieceListString(items) {
                const itemStrings = items.map(item => item.qName);
                if (!items || items.length === 0) return "";
                if (items.length === 1) return itemStrings[0];
                if (items.length === 2) return itemStrings.join(" and ");
                return itemStrings.slice(0, -1).join(", ") + ", and " + itemStrings[items.length - 1];
            }
        }

        // Weather info block
        Loader {
            // only show the weather block if there is weather data available
            active: !!model.qWeather
            visible: active
            sourceComponent: Rectangle {
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
                                text: `${model.qWeather.degrees_celcius}°C`
                            }
                            Text {
                                text: ` | `
                            }
                            Text {
                                text: model.qWeather.cloudiness
                            }
                        }

                        RowLayout {
                            Text {
                                text: model.qWeather.city
                            }
                            Text {
                                text: " | "
                            }
                            Text {
                                text: `${model.qWeather.min_daily_temp_celcius}-${model.qWeather.max_daily_temp_celcius}°C`
                            }
                            Text {
                                text: " | "
                            }
                            Text {
                                text: `Rain ${model.qWeather.rain_chance * 100}%`
                            }
                        }
                    }
                }
            }
        }

        // Event info block
        Loader {
            // Only show event info if there is one using this outfit
            active: !!model.qEvent
            visible: active
            sourceComponent: Rectangle {
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
                        text: model.qEvent.qName
                    }
                    Text {
                        text: " | "
                    }
                    Text {
                        text: OutfitFormality.qDisplayName(model.qEvent.qFormality)
                    }
                }
            }
        }

        // Rating buttons
        RowLayout {
            Layout.fillWidth: true
            id: rating_buttons_layout
            visible: !root.decided
            Button {
                Layout.fillWidth: true
                text: "Not for me"
                onClicked: root.notForMe()
            }
            Button {
                Layout.fillWidth: true
                text: "Wear today"
                onClicked: root.wearToday()
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.decided
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            text: "Great choice! Enjoy your outfit."
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
