import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Vestir

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
                text: recommendationPageModel.error !== ""
                      ? qsTr("Couldn't get outfits.")
                      : recommendationPageModel.closetEmpty
                        ? qsTr("Your closet is empty.")
                        : recommendationPageModel.noMore
                          ? qsTr("You've seen all the outfits for now.")
                          : qsTr("Finding outfits...")
            }

            Text {
                Layout.fillWidth: true
                visible: recommendationPageModel.noMore
                         || recommendationPageModel.closetEmpty
                         || recommendationPageModel.error !== ""
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                font: Theme.font.caption
                color: Theme.colours.alt_c2
                text: recommendationPageModel.error !== ""
                      ? qsTr("Check that the ML server is running, then try again. (%1)").arg(recommendationPageModel.error)
                      : recommendationPageModel.closetEmpty
                        ? qsTr("Add some clothes in the Closet tab to get outfit suggestions.")
                        : qsTr("Add more clothes for more variety, or start over to see them again.")
            }

            StyledButtonDark {
                Layout.alignment: Qt.AlignHCenter
                visible: recommendationPageModel.noMore
                         || recommendationPageModel.error !== ""
                text: recommendationPageModel.error !== "" ? qsTr("Try again") : qsTr("Start over")
                onClicked: recommendationPageModel.refresh()
            }

            Item { Layout.fillHeight: true }
        }
    }
}
