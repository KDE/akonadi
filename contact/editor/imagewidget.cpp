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

#include "imagewidget.h"

#include <kabc/addressee.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <kicon.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kpixmapregionselectordialog.h>

#include <QtCore/QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>

/**
 * @short Small helper class to load image from network
 */
class ImageLoader
{
public:
    ImageLoader(QWidget *parent = 0);

    QImage loadImage(const KUrl &url, bool *ok);

private:
    QWidget *mParent;
};

ImageLoader::ImageLoader(QWidget *parent)
    : mParent(parent)
{
}

QImage ImageLoader::loadImage(const KUrl &url, bool *ok)
{
    QImage image;
    QString tempFile;

    if (url.isEmpty()) {
        return image;
    }

    (*ok) = false;

    if (url.isLocalFile()) {
        if (image.load(url.toLocalFile())) {
            (*ok) = true;
        }
    } else if (KIO::NetAccess::download(url, tempFile, mParent)) {
        if (image.load(tempFile)) {
            (*ok) = true;
        }
        KIO::NetAccess::removeTempFile(tempFile);
    }

    if (!(*ok)) {
        // image does not exist (any more)
        KMessageBox::sorry(mParent, i18n("This contact's image cannot be found."));
        return image;
    }

    QPixmap pixmap = QPixmap::fromImage(image);

    image = KPixmapRegionSelectorDialog::getSelectedImage(pixmap, 1, 1, mParent);
    if (image.isNull()) {
        (*ok) = false;
        return image;
    }

    if (image.height() > 720 || image.width() > 720) {
        if (image.height() > image.width()) {
            image = image.scaledToHeight(720);
        } else {
            image = image.scaledToWidth(720);
        }
    }

    (*ok) = true;

    return image;
}

ImageWidget::ImageWidget(Type type, QWidget *parent)
    : QPushButton(parent)
    , mType(type)
    , mHasImage(false)
    , mReadOnly(false)
    , mImageLoader(0)
{
    setAcceptDrops(true);

    setIconSize(QSize(100, 100));
    setFixedSize(QSize(120, 120));

    connect(this, SIGNAL(clicked()), SLOT(changeImage()));

    if (mType == Photo) {
        setToolTip(i18n("The photo of the contact (click to change)"));
    } else {
        setToolTip(i18n("The logo of the company (click to change)"));
    }

    updateView();
}

ImageWidget::~ImageWidget()
{
    delete mImageLoader;
}

void ImageWidget::loadContact(const KABC::Addressee &contact)
{
    mPicture = (mType == Photo ? contact.photo() : contact.logo());
    if (mPicture.isIntern() && !mPicture.data().isNull()) {
        mHasImage = true;
    }

    updateView();
}

void ImageWidget::storeContact(KABC::Addressee &contact) const
{
    if (mType == Photo) {
        contact.setPhoto(mPicture);
    } else {
        contact.setLogo(mPicture);
    }
}

void ImageWidget::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;
}

void ImageWidget::updateView()
{
    if (mHasImage) {
        setIcon(QPixmap::fromImage(mPicture.data()));
    } else {
        if (mType == Photo) {
            setIcon(KIcon(QLatin1String("user-identity")));
        } else {
            setIcon(KIcon(QLatin1String("image-x-generic")));
        }
    }
}

void ImageWidget::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    event->setAccepted(mimeData->hasImage() || mimeData->hasUrls());
}

void ImageWidget::dropEvent(QDropEvent *event)
{
    if (mReadOnly) {
        return;
    }

    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasImage()) {
        mPicture.setData(qvariant_cast<QImage>(mimeData->imageData()));
        mHasImage = true;
        updateView();
    }

    const KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    if (urls.isEmpty()) {   // oops, no data
        event->setAccepted(false);
    } else {
        bool ok = false;
        const QImage image = imageLoader()->loadImage(urls.first(), &ok);
        if (ok) {
            mPicture.setData(image);
            mHasImage = true;
            updateView();
        }
    }
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    mDragStartPos = event->pos();
    QPushButton::mousePressEvent(event);
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) &&
        (event->pos() - mDragStartPos).manhattanLength() > KGlobalSettings::dndEventDelay()) {

        if (mHasImage) {
            QDrag *drag = new QDrag(this);
            drag->setMimeData(new QMimeData());
            drag->mimeData()->setImageData(mPicture.data());
            drag->start();
        }
    }
}

void ImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu;

    if (mType == Photo) {
        if (!mReadOnly) {
            menu.addAction(i18n("Change photo..."), this, SLOT(changeImage()));
        }

        if (mHasImage) {
            menu.addAction(i18n("Save photo..."), this, SLOT(saveImage()));

            if (!mReadOnly) {
                menu.addAction(i18n("Remove photo"), this, SLOT(deleteImage()));
            }
        }
    } else {
        if (!mReadOnly) {
            menu.addAction(i18n("Change logo..."), this, SLOT(changeImage()));
        }

        if (mHasImage) {
            menu.addAction(i18n("Save logo..."), this, SLOT(saveImage()));

            if (!mReadOnly) {
                menu.addAction(i18n("Remove logo"), this, SLOT(deleteImage()));
            }
        }
    }

    menu.exec(event->globalPos());
}

void ImageWidget::changeImage()
{
    if (mReadOnly) {
        return;
    }

    const KUrl url = KFileDialog::getOpenUrl(QUrl(), KImageIO::pattern(), this);
    if (url.isValid()) {
        bool ok = false;
        const QImage image = imageLoader()->loadImage(url, &ok);
        if (ok) {
            mPicture.setData(image);
            mHasImage = true;
            updateView();
        }
    }
}

void ImageWidget::saveImage()
{
    const QString fileName = KFileDialog::getSaveFileName(KUrl(), KImageIO::pattern(), this, QString(), KFileDialog::ConfirmOverwrite);
    if (!fileName.isEmpty()) {
        mPicture.data().save(fileName);
    }
}

void ImageWidget::deleteImage()
{
    mHasImage = false;
    mPicture.setData(QImage());
    updateView();
}

ImageLoader *ImageWidget::imageLoader()
{
    if (!mImageLoader) {
        mImageLoader = new ImageLoader;
    }

    return mImageLoader;
}
