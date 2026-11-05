import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

Page {
    CalendarModel {
        id: calendar
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        Text {
            text: qsTr("Calendar link (iCal)")
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            font.pixelSize: 12
            text: qsTr("Google Calendar: Settings > your calendar > Integrate calendar > Secret address in iCal format.")
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: linkField
                Layout.fillWidth: true
                placeholderText: "https://calendar.google.com/.../basic.ics"
                text: calendar.icsUrl
                selectByMouse: true
            }
            Button {
                text: qsTr("Sync")
                enabled: !calendar.loading
                onClicked: calendar.sync(linkField.text)
            }
        }

        Text {
            visible: text.length > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: calendar.message
        }

        Text {
            text: qsTr("Add an event")
            font.bold: true
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("Name")
            }
            ComboBox {
                id: formalityBox
                model: ["casual", "smart-casual", "formal", "sporty"]
            }
        }
        RowLayout {
            Layout.fillWidth: true
            TextField {
                id: dateField
                Layout.fillWidth: true
                text: Qt.formatDate(new Date(), "yyyy-MM-dd")
            }
            TextField {
                id: timeField
                Layout.preferredWidth: 80
                text: "12:00"
            }
            Button {
                text: qsTr("Add")
                onClicked: if (calendar.addEvent(nameField.text, dateField.text, timeField.text, formalityBox.currentText))
                    nameField.clear()
            }
        }

        Text {
            text: qsTr("Next 7 days")
            font.bold: true
        }
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 6
            model: calendar.events
            delegate: RowLayout {
                required property var modelData
                width: ListView.view.width
                ColumnLayout {
                    Layout.fillWidth: true
                    Text {
                        text: modelData.name
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        font.pixelSize: 12
                        text: modelData.when + " | " + modelData.formality
                    }
                }
                Button {
                    text: "x"
                    onClicked: calendar.removeEvent(modelData.id)
                }
            }
            Text {
                anchors.centerIn: parent
                visible: parent.count === 0
                text: qsTr("No events")
            }
        }
    }
}
