/*
    This file is part of KAddressBook.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef SOUNDEDITWIDGET_H
#define SOUNDEDITWIDGET_H

#include <QtCore/QPoint>
#include <QtGui/QToolButton>

namespace KABC
{
class Addressee;
}

class SoundLoader;

class SoundEditWidget : public QToolButton
{
  Q_OBJECT

  public:
    SoundEditWidget( QWidget *parent = 0 );
    ~SoundEditWidget();

    void loadContact( const KABC::Addressee &contact );
    void storeContact( KABC::Addressee &contact ) const;

    void setReadOnly( bool readOnly );

  protected:
    // context menu handling
    virtual void contextMenuEvent( QContextMenuEvent* );

  private Q_SLOTS:
    void playSound();
    void updateView();

    void changeSound();
    void saveSound();
    void deleteSound();

  private:
    SoundLoader *soundLoader();

    QByteArray mSound;
    bool mHasSound;
    bool mReadOnly;

    SoundLoader *mSoundLoader;
};

#endif
