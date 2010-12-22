import Qt 4.7 as QML
import org.kde.pim.mobileui 4.5 as KPIM

QML.Rectangle {
  width : 800
  height : 480
  color : "#00000000"

  QML.Text {
    id : descriptionText
    anchors.top : parent.top
    anchors.left : parent.left
    height : text == "" ? 0 : 20
    text : dialogController.descriptionText
  }

  QML.ListView {
    id : listView
    anchors.top : descriptionText.bottom
    anchors.topMargin : 20
    anchors.left : parent.left
    anchors.right : parent.right
    anchors.bottom : filterLine.top
    clip : true
    boundsBehavior : QML.Flickable.StopAtBounds

    model : collectionModel

    delegate : QML.Rectangle {
      width : listView.width
      height : 35

      QML.Rectangle {
        anchors.fill : parent
        color : "lightsteelblue"
        opacity : QML.ListView.isCurrentItem ? 0.25 : 0
        radius : 10
      }

      QML.Text {
        anchors.fill : parent
        anchors.leftMargin : 15
        text : model.display
        verticalAlignment : QML.Text.AlignVCenter
      }

      QML.MouseArea {
        anchors.fill : parent
        onClicked : listView.currentIndex = model.index
      }
    }

    QML.Connections {
      target : dialogController

      onSelectionChanged : listView.currentIndex = row
    }

    onCurrentIndexChanged : dialogController.setCurrentIndex( currentIndex )
  }

  QML.TextInput {
    id : filterLine
    anchors.left : parent.left
    anchors.right : parent.right
    anchors.bottom : okButton.top

    focus : true
    height : text == "" ? 0 : 20
    opacity : text == "" ? 0 : 1

    onTextChanged : dialogController.setFilterText( text )
  }

  KPIM.Button2 {
    id : createButton
    anchors.right : okButton.left
    anchors.rightMargin : 100
    anchors.bottom : parent.bottom
    width : 150

    buttonText : createButtonText
    onClicked : dialogController.createClicked()
    enabled : dialogController.createButtonEnabled
    visible : dialogController.createButtonVisible
  }

  KPIM.Button2 {
    id : okButton
    anchors.right : cancelButton.left
    anchors.bottom : parent.bottom
    width : 150

    buttonText : okButtonText
    onClicked : dialogController.okClicked()
    enabled : dialogController.okButtonEnabled
  }

  KPIM.Button2 {
    id: cancelButton
    anchors.right : parent.right
    anchors.bottom : parent.bottom
    width : 150

    buttonText : cancelButtonText
    onClicked : dialogController.cancelClicked()
    enabled : dialogController.cancelButtonEnabled
  }
}
