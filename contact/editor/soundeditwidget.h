/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef SOUNDEDITWIDGET_H
#define SOUNDEDITWIDGET_H

#include <QToolButton>

namespace KABC
{
class Addressee;
}

class SoundLoader;

class SoundEditWidget : public QToolButton
{
    Q_OBJECT

public:
    explicit SoundEditWidget(QWidget *parent = 0);
    ~SoundEditWidget();

    void loadContact(const KABC::Addressee &contact);
    void storeContact(KABC::Addressee &contact) const;

    void setReadOnly(bool readOnly);

protected:
    // context menu handling
    virtual void contextMenuEvent(QContextMenuEvent *event);

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
