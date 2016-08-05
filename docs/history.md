# Historical background {#history}

## General

During the last 5 years, after the release of KDE 3.0, the requirements of our users
have constantly increased. While it was sufficient that our PIM solution was able to handle 100 contacts,
300 events and maybe 1000 mails in 2001, nowadays users expect the software to be able to
handle a multiple of that. Over the years, the KDE PIM developers tried to catch up with the new
requirements; however, since KDE 3.x had to stay binary compatible, they were limited in their
efforts.

With the new major release KDE 4.0 it's possible to completely redesign the PIM libraries from
the ground up and use new concepts to face the requirements of 2006 and beyond.

After some discussion at the annual KDE PIM meeting in Osnabrück in January 2006, the PIM developers
came to the conclusion that a service is needed which acts as a local cache on the user's desktop
and provides search facilities. The name Akonadi comes from a divinity from Ghana and was chosen since
all other nice names were already used by other projects on the Internet ;)

## Problems with the implementation of KDE 3.x

Before digging into the internals of Akonadi, we want to take a look at the implementation of the
old KDE PIM libraries to understand the problems and conceptual shortcomings.

The main PIM libraries libkabc (contacts) and libkcal (events) where designed at a time when the
address book and calendar were files on the local file system, so there was no reason to think
about access time and mode. The libraries accessed the files synchronously and loaded all data of the
file into memory to be able to perform search queries on the data set. It worked well for local files,
but over time plug-ins for loading data from groupware servers were written, so the synchronous access blocked
applications which used libkabc/libkcal, and loading all 2000 contacts from a server is not only
time consuming but also needs a lot of memory to store them locally. The KDE PIM developers tried to
address the first issue by adding an asynchronous API, but it was not well implemented and was difficult to use.
In the end, the design decisions caused the following problems:

* Bad Performance
* High Memory Consumption

Another important but missing thing in the libraries was support for notifications and locking.
The former was partly implemented (at least reflected by the API) but only implemented in the local
file plug-in, so it was in practice unusable. The latter was also partly implemented but never really tested and
lead to deadlocks sometimes, so the following problems appeared as well:

* Missing Notifications
* Missing Locking

The main aim of Akonadi is to solve these issues and make use of the goodies which the new design brings.
