#include "helper.h"

#include <akonadi/itemfetchjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <QLatin1String>
#include <QStringList>

using namespace Akonadi;

bool Helper::confirmExists(const Akonadi::Item &item)
{
    ItemFetchJob *job = new ItemFetchJob(item);
    return job->exec() != 0;
}

bool Helper::confirmDoesntExist(const Akonadi::Item &item)
{
    ItemFetchJob *job = new ItemFetchJob(item);
    return job->exec() == 0;
}


Akonadi::Collection Helper::fetchCollection()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(),
                                                     CollectionFetchJob::Recursive);
    // Get list of collections
    job->fetchScope().setContentMimeTypes(QStringList() << QLatin1String("application/x-vnd.akonadi.calendar.event"));
    const bool ret = job->exec();
    Q_ASSERT(ret);

    // Find our collection
    Collection::List collections = job->collections();
    Collection collection = collections.first();

    Q_ASSERT(collection.isValid());

    return collection;
}
