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

#include "image-provider-tests.h"
#include "../imageprovider.h"

#include <KImageCache>

#include <QImage>

#define IMAGE_URL "http://twimg0-a.akamaihd.net/profile_images/1171007559/79ac2ace-f87d-4dce-ac6c-825b6b12729d.png"

void ImageProviderTests::setup()
{
    //use special cache
    KImageCache *cache = new KImageCache( QLatin1String( "asu_tests_cache" ), 10485760 );
    cache->clear();

    QEventLoop e;

    Akonadi::ImageProvider i;
    QImage image = i.loadImage( QLatin1String( "mck182" ),
                                KUrl( IMAGE_URL ),
                                true,
                                cache );

    connect(&i, SIGNAL(imageLoaded(QString,KUrl,QImage)),
            this, SLOT(onImageFetched(QString,KUrl,QImage)));
    connect(&i, SIGNAL(imageLoaded(QString,KUrl,QImage)),
            &e, SLOT(quit()));

    //wait for the fetcher to finish
    e.exec();
}

void ImageProviderTests::testImageNoCache()
{
    KImageCache *cache = new KImageCache( QLatin1String( "asu_tests_cache" ), 10485760 );
    cache->clear();

    Akonadi::ImageProvider i;
    QImage image = i.loadImage( QLatin1String( "mck182" ),
                                KUrl( IMAGE_URL ),
                                true,
                                cache);

    QVERIFY( image.isNull() );
}

void ImageProviderTests::testImageInCache()
{
    setup();

    QVERIFY( !m_image.isNull() );
}

void ImageProviderTests::onImageFetched( const QString &who, const KUrl &url, QImage image )
{
    m_image = image;
    m_name = who;
    m_url = url;
}

void ImageProviderTests::testImageFetchingImage()
{
    setup();

    QVERIFY( !m_image.isNull() );
}

void ImageProviderTests::testImageFetchingName()
{
    setup();

    QCOMPARE( m_name, QLatin1String( "mck182" ) );
}

void ImageProviderTests::testImageFetchingUrl()
{
    setup();

    QCOMPARE( m_url, KUrl( IMAGE_URL ) );
}

QTEST_KDEMAIN( ImageProviderTests, GUI );

