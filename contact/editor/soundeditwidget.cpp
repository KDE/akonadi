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

#include "soundeditwidget.h"

#include <kabc/addressee.h>
#include <kfiledialog.h>
#include <kicon.h>
#include <kio/netaccess.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <phonon/mediaobject.h>

#include <QtCore/QBuffer>
#include <QContextMenuEvent>
#include <QMenu>

/**
 * @short Small helper class to load sound from network
 */
class SoundLoader
{
public:
    SoundLoader(QWidget *parent = 0);

    QByteArray loadSound(const KUrl &url, bool *ok);

private:
    QByteArray mSound;
    QWidget *mParent;
};

SoundLoader::SoundLoader(QWidget *parent)
    : mParent(parent)
{
}

QByteArray SoundLoader::loadSound(const KUrl &url, bool *ok)
{
    QByteArray sound;
    QString tempFile;

    if (url.isEmpty()) {
        return sound;
    }

    (*ok) = false;

    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        if (file.open(QIODevice::ReadOnly)) {
            sound = file.readAll();
            file.close();
            (*ok) = true;
        }
    } else if (KIO::NetAccess::download(url, tempFile, mParent)) {
        QFile file(tempFile);
        if (file.open(QIODevice::ReadOnly)) {
            sound = file.readAll();
            file.close();
            (*ok) = true;
        }
        KIO::NetAccess::removeTempFile(tempFile);
    }

    if (!(*ok)) {
        KMessageBox::sorry(mParent, i18n("This contact's sound cannot be found."));
        return sound;
    }

    (*ok) = true;

    return sound;
}

SoundEditWidget::SoundEditWidget(QWidget *parent)
    : QToolButton(parent)
    , mHasSound(false)
    , mReadOnly(false)
    , mSoundLoader(0)
{
    connect(this, SIGNAL(clicked()), SLOT(playSound()));

    updateView();
}

SoundEditWidget::~SoundEditWidget()
{
    delete mSoundLoader;
}

void SoundEditWidget::loadContact(const KABC::Addressee &contact)
{
    const KABC::Sound sound = contact.sound();
    if (sound.isIntern() && !sound.data().isEmpty()) {
        mHasSound = true;
        mSound = sound.data();
    }

    updateView();
}

void SoundEditWidget::storeContact(KABC::Addressee &contact) const
{
    KABC::Sound sound(contact.sound());
    sound.setData(mSound);
    contact.setSound(sound);
}

void SoundEditWidget::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;
}

void SoundEditWidget::updateView()
{
    if (mHasSound) {
        setIcon(KIcon(QLatin1String("audio-volume-medium")));
        setToolTip(i18n("Click to play pronunciation"));
    } else {
        setIcon(KIcon(QLatin1String("audio-volume-muted")));
        setToolTip(i18n("No pronunciation available"));
    }
}

void SoundEditWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    if (mHasSound) {
        menu.addAction(i18n("Play"), this, SLOT(playSound()));
    }

    if (!mReadOnly) {
        menu.addAction(i18n("Change..."), this, SLOT(changeSound()));
    }

    if (mHasSound) {
        menu.addAction(i18n("Save..."), this, SLOT(saveSound()));

        if (!mReadOnly) {
            menu.addAction(i18n("Remove"), this, SLOT(deleteSound()));
        }
    }

    menu.exec(event->globalPos());
}

void SoundEditWidget::playSound()
{
    if (!mHasSound) {
        return;
    }

    Phonon::MediaObject *player = Phonon::createPlayer(Phonon::NotificationCategory);
    QBuffer *soundData = new QBuffer(player);
    soundData->setData(mSound);
    player->setCurrentSource(soundData);
    player->setParent(this);
    connect(player, SIGNAL(finished()), player, SLOT(deleteLater()));
    player->play();
}

void SoundEditWidget::changeSound()
{
    const KUrl url = KFileDialog::getOpenUrl(QUrl(), QLatin1String("*.wav"), this);
    if (url.isValid()) {
        bool ok = false;
        const QByteArray sound = soundLoader()->loadSound(url, &ok);
        if (ok) {
            mSound = sound;
            mHasSound = true;
            updateView();
        }
    }
}

void SoundEditWidget::saveSound()
{
    const QString fileName = KFileDialog::getSaveFileName(KUrl(), QLatin1String("*.wav"), this, QString(), KFileDialog::ConfirmOverwrite);
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(mSound);
            file.close();
        }
    }
}

void SoundEditWidget::deleteSound()
{
    mHasSound = false;
    mSound = QByteArray();
    updateView();
}

SoundLoader *SoundEditWidget::soundLoader()
{
    if (!mSoundLoader) {
        mSoundLoader = new SoundLoader;
    }

    return mSoundLoader;
}
