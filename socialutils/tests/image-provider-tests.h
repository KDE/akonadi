/*
    Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2.1 of the License, or (at your
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

#ifndef IMAGE_PROVIDER_TESTS_H
#define IMAGE_PROVIDER_TESTS_H

#include <QtTest/QTest>
#include <QtCore/QObject>
#include <qtest_kde.h>

#include <QImage>
#include <KUrl>

class ImageProviderTests : public QObject
{
Q_OBJECT

public Q_SLOTS:
    void onImageFetched(const QString &who, const KUrl &url, QImage image);

private Q_SLOTS:
    void testImageNoCache();
    void testImageInCache();
    void testImageFetchingImage();
    void testImageFetchingName();
    void testImageFetchingUrl();

private:
    void setup();

    QImage m_image;
    QString m_name;
    KUrl m_url;
};

#endif // IMAGE_PROVIDER_TESTS_H
