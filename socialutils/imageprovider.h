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

#ifndef AKONADI_SOCIALUTILS_IMAGEPROVIDER_H
#define AKONADI_SOCIALUTILS_IMAGEPROVIDER_H

#include "libakonadisocialutils_export.h"

#include <KDE/KUrl>

class KJob;
class QImage;
class KImageCache;

namespace KIO {
  class Job;
}

namespace Akonadi {

class ImageProviderPrivate;

/**
 * Class fetching avatars/images from network and storing them in KImageCache
 */
class LIBAKONADISOCIALUTILS_EXPORT ImageProvider : public QObject
{
  Q_OBJECT
  public:
    explicit ImageProvider( QObject *parent = 0 );
    ~ImageProvider();

    /**
     * Starts fetching the avatar/image from network
     *
     * @param who The username this picture belongs to, can be some general id as well
     * @param url URL of the image
     * @param polishImage set to true if you want the image to have rounded corners,
     *        used for avatars mainly
     */
    QImage loadImage( const QString &who, const KUrl &url,
                      bool polishImage = false, KImageCache *cache = 0 );

    /**
     * Aborts all running jobs
     */
    void abortAllJobs();

  Q_SIGNALS:
    /**
     * Signals image loading has finished
     *
     * @param who The username this picture belongs to
     * @param url URL of the image
     * @param image The image itself
     */
    void imageLoaded( const QString &who, const KUrl &url, const QImage &image );

  private:
    ImageProviderPrivate * const d_ptr;
    Q_DECLARE_PRIVATE( ImageProvider )
    Q_PRIVATE_SLOT( d_func(), void result( KJob *job ) )
    Q_PRIVATE_SLOT( d_func(), void recv( KIO::Job *job, const QByteArray &data ) )
};

}

#endif // AKONADISOCIALUTILS_IMAGEPROVIDER_H
