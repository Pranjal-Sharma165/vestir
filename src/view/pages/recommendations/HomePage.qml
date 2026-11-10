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

        Text {
            Layout.fillWidth: true
            Layout.fillHeight: recommendationPageModel.current === null
            visible: recommendationPageModel.current === null

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap

            font: Theme.font.body
            color: Theme.colours.text_alt

            text: recommendationPageModel.noMore
                  ? "No more outfits to suggest for now."
                  : "Finding outfits..."
        }
    }
}
