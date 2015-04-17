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

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include "picture.h"

#include <QtCore/QPoint>
#include <QPushButton>

namespace KABC
{
class Addressee;
}

class ImageLoader;

class ImageWidget : public QPushButton
{
    Q_OBJECT

public:
    enum Type {
        Photo,
        Logo
    };

    explicit ImageWidget(Type type, QWidget *parent = 0);
    ~ImageWidget();

    void loadContact(const KABC::Addressee &contact);
    void storeContact(KABC::Addressee &contact) const;

    void setReadOnly(bool readOnly);

protected:
    // image drop handling
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

    // image drag handling
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

    // context menu handling
    virtual void contextMenuEvent(QContextMenuEvent *event);

private Q_SLOTS:
    void updateView();

    void changeImage();
    void saveImage();
    void deleteImage();

private:
    ImageLoader *imageLoader();

    KABC::Picture mPicture;
    QPoint mDragStartPos;
    ImageLoader *mImageLoader;
    Type mType;
    bool mHasImage;
    bool mReadOnly;
};

#endif
