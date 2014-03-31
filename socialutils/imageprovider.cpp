/*
  Avatar provider - based on Microblog dataengine implementation
  Copyright (C) 2008  Aaron Seigo <aseigo@kde.org>
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

#include "imageprovider.h"

#include <KDebug>
#include <KIO/Job>
#include <KImageCache>

#include <QPainter>

class Akonadi::ImageProviderPrivate
{
  Q_DECLARE_PUBLIC( ImageProvider )

  public:

    struct QueuedJobHelper {
      QString who;
      KUrl url;
      bool polishImage;
    };

    ImageProviderPrivate( ImageProvider *q ) : q_ptr( q )
    {
    }

    /**
     * Makes the corners of the image rounded
     */
    QImage polishImage( const QImage &img )
    {
      const int sz = 48 * 4;
      QImage roundedImage = QImage( QSize( sz, sz ), QImage::Format_ARGB32_Premultiplied );
      roundedImage.fill( Qt::transparent );
      QPainter p;
      p.begin( &roundedImage );
      QPainterPath clippingPath;
      QRectF imgRect = QRectF( QPoint( 0, 0 ), roundedImage.size() );
      clippingPath.addRoundedRect( imgRect, 24, 24 );
      p.setClipPath( clippingPath );
      p.setClipping( true );
      p.drawImage( QRectF( QPointF( 0, 0 ), roundedImage.size() ), img );
      return roundedImage;
    }

    ///All active jobs
    QHash<KJob *, QString> jobs;

    ///Received data from the jobs
    QHash<KJob *, QByteArray> jobData;

    ///Number of running jobs
    int runningJobs;

    ///Queued jobs when the limit is reached
    QList<QueuedJobHelper> queuedJobs;

    ///String list containing the list of loaded persons avatars
    QStringList pendingPersons;

    ///KImageCache instance
    KImageCache *imageCache;

  private:
    /**
     * This slot is called when fetching the avatar has finished
     */
    void result( KJob *job );

    /**
     * Handling received data from KJob
     */
    void recv( KIO::Job *job, const QByteArray &data );

    ImageProvider *q_ptr;
};

void Akonadi::ImageProviderPrivate::recv( KIO::Job *job, const QByteArray &data )
{
  jobData[job] += data;
}

void Akonadi::ImageProviderPrivate::result( KJob *job )
{
  Q_Q( ImageProvider );
  if ( !jobs.contains( job ) ) {
    qDebug() << "Tried to handle unknown job, returning...";
    return;
  }

  runningJobs--;

  if ( queuedJobs.count() > 0 ) {
    QueuedJobHelper helper = queuedJobs.takeFirst();
    q->loadImage( helper.who, helper.url, helper.polishImage );
  }

  if ( job->error() ) {
    // TODO: error handling
    KIO::TransferJob* kiojob = dynamic_cast<KIO::TransferJob*>( job );
    qCritical() << "Image job for" << jobs.value( job ) << "returned error:" << kiojob->errorString();
  } else {
    const QString who = jobs.value( job );

    QImage image;
    image.loadFromData( jobData.value( job ) );
    KIO::TransferJob* kiojob = dynamic_cast<KIO::TransferJob*>( job );
    const QString cacheKey = who + QLatin1Char( '@' ) +
                             kiojob->property( "imageUrl" ).value<KUrl>().pathOrUrl();

    qDebug() << "Downloaded image for" << who << "(key:" << cacheKey << ")";

    imageCache->insertImage( cacheKey, image );
    pendingPersons.removeAll( cacheKey );

    bool polishImage = job->property( "polishImage" ).toBool();

    Q_EMIT q->imageLoaded( who,
                           kiojob->property( "imageUrl" ).value<KUrl>(),
                           polishImage ? this->polishImage( image ) : image );
  }

  jobs.remove( job );
  jobData.remove( job );
}

Akonadi::ImageProvider::ImageProvider( QObject *parent )
  : QObject( parent ),
    d_ptr( new ImageProviderPrivate( this ) )
{
  Q_D( ImageProvider );
  d->imageCache = 0;
  d->runningJobs = 0;
}

Akonadi::ImageProvider::~ImageProvider()
{
  Q_D( ImageProvider );
  delete d;
}

QImage Akonadi::ImageProvider::loadImage( const QString &who, const KUrl &url,
                                          bool polishImage, KImageCache *cache )
{
  Q_D( ImageProvider );

  if ( who.isEmpty() ) {
    return QImage();
  }

  if ( !d->imageCache && !cache ) {
    //if no old cache and no passed cache, default to plasma_engine_preview
    d->imageCache = new KImageCache( QLatin1String( "plasma_engine_preview" ),
                                     10485760 ); // Re-use previewengine's cache
  } else if ( !d->imageCache && cache ) {
    //if there is no old cache, set the new one
    d->imageCache = cache;
  } else if ( d->imageCache && cache ) {
    //delete old one and set new one
    //delete d->imageCache; //FIXME: crashes
    d->imageCache = cache;
  }

  const QString cacheKey = who + QLatin1Char( '@' ) + url.pathOrUrl();

  // Make sure we only start one job per user
  if ( d->pendingPersons.contains( cacheKey ) ) {
    qDebug() << "Job for" << who << "already running, returning";
    return QImage();
  }

  // Check if the image is in the cache, if so emit it and return
  QImage preview;
  preview.fill( Qt::transparent );

  if ( d->imageCache->findImage( cacheKey, &preview ) ) {
    // cache hit
    qDebug() << "Image for" << who << "already in cache, returning it";
    return polishImage ? d->polishImage( preview ) : preview;
  }

  if ( !url.isValid() ) {
    qDebug() << "Invalid url, returning";
    return QImage();
  }

  qDebug() << "No cache, fetching image for" << who;

  d->pendingPersons << cacheKey;
  //FIXME: since kio_http bombs the system with too many request put a temporary
  // arbitrary limit here, revert as soon as BUG 192625 is fixed
  // Note: seems fixed.
  if ( d->runningJobs < 500 ) {
    d->runningJobs++;
    qDebug() << "Starting fetch job for" << who;
    KIO::Job *job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );
    job->setAutoDelete( true );
    d->jobs[job] = who;
    connect( job, SIGNAL(data(KIO::Job*,QByteArray)),
             this, SLOT(recv(KIO::Job*,QByteArray)) );
    connect( job, SIGNAL(result(KJob*)),
             this, SLOT(result(KJob*)) );
    // The url needs to be stored explicitly because, for example, facebook redirects
    // randomly between its servers causing the url to be different every time, which
    // makes storing the job's url in cache impossible therefore we set it here and
    // use this when saving to the cache instead of job->url()
    job->setProperty( "imageUrl", url );
    job->setProperty( "polishImage", polishImage );
    job->start();
  } else {
    qDebug() << "Queuing job for" << who;
    ImageProviderPrivate::QueuedJobHelper helper;
    helper.who = who;
    helper.url = url;
    helper.polishImage = polishImage;
    d->queuedJobs.append( helper );
  }

  return QImage();
}

void Akonadi::ImageProvider::abortAllJobs()
{
  Q_D( ImageProvider );
  Q_FOREACH ( KJob *job, d->jobs.keys() ) {
    job->kill();
    d->jobs.remove( job );
    d->jobData.remove( job );
  }
}

#include "moc_imageprovider.cpp"
