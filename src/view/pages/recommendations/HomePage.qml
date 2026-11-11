import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir
import Vestir.DesignTokens as Tokens

Page {
    id: root
    required property RecommendationPageModel recommendationPageModel

    background: Rectangle {
        color: Theme.colours.background_c2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.pageTop
        anchors.bottomMargin: Theme.spacing.pageBottom
        anchors.leftMargin: Theme.spacing.pageSides
        anchors.rightMargin: anchors.leftMargin

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            visible: recommendationPageModel.current !== null

            Loader {
                active: !!recommendationPageModel.current
                visible: active
                sourceComponent: RecommendationElement {
                    width: scrollView.availableWidth
                    model: recommendationPageModel.current
                    decided: recommendationPageModel.chosen
                    onNotForMe: recommendationPageModel.notForMe()
                    onWearToday: recommendationPageModel.choose()
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: recommendationPageModel.current === null
            visible: recommendationPageModel.current === null
            spacing: 16

            Item { Layout.fillHeight: true }

            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font: Theme.font.subtitle
                color: Theme.colours.alt_c2
                text: recommendationPageModel.noMore
                      ? qsTr("You've seen all the outfits for now.")
                      : qsTr("Finding outfits...")
            }

            Text {
                Layout.fillWidth: true
                visible: recommendationPageModel.noMore
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font: Theme.font.caption
                color: Theme.colours.alt_c2
                text: qsTr("Add more clothes for more variety, or start over to see them again.")
            }

            StyledButtonDark {
                Layout.alignment: Qt.AlignHCenter
                visible: recommendationPageModel.noMore
                text: qsTr("Start over")
                onClicked: recommendationPageModel.startOver()
            }

            Item { Layout.fillHeight: true }
        }
    }
}
