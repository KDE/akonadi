/*
    Copyright 2010 Tobias Koenig <tokoe@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/
import Qt 4.7 as QML
import org.kde.pim.mobileui 4.5 as KPIM

QML.Rectangle {
  width : 780
  height : 460
  color : palette.window

  QML.SystemPalette {
    id: palette
    colorGroup: QML.SystemPalette.Active
  }

  QML.Text {
    id : descriptionText
    anchors.top : parent.top
    anchors.left : parent.left
    anchors.leftMargin : 15
    height : text == "" ? 0 : 20
    text : dialogController.descriptionText
    font.bold : true
    verticalAlignment : QML.Text.AlignVCenter
  }

  QML.ListView {
    id : listView
    anchors.top : descriptionText.bottom
    anchors.topMargin : 20
    anchors.left : parent.left
    anchors.leftMargin : 15
    anchors.right : parent.right
    anchors.rightMargin : 15
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
        opacity : QML.ListView.isCurrentItem ? 0.30 : 0
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
    anchors.topMargin : 5
    anchors.left : parent.left
    anchors.leftMargin : 20
    anchors.right : parent.right
    anchors.rightMargin : 20
    anchors.bottom : okButton.top
    anchors.bottomMargin : 5

    focus : true
    height : text == "" ? 0 : 20
    opacity : text == "" ? 0 : 1

    onTextChanged : dialogController.setFilterText( text )

    QML.Rectangle {
      anchors.fill : parent
      anchors.leftMargin : -5
      anchors.rightMargin : -5
      z: parent.z - 1
      color : "lightsteelblue"
      opacity : 0.50
      radius : 5
    }
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
    anchors.rightMargin: 15
    anchors.bottom : parent.bottom
    width : 150

    buttonText : cancelButtonText
    onClicked : dialogController.cancelClicked()
    enabled : dialogController.cancelButtonEnabled
  }
}
