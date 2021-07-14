# Tags # {#tags}

[TOC]

Akonadi tags are an abstract concept designed to allow attaching the same shared
data to multiple Akonadi Items, rather than just grouping of arbitrary Akonadi Items
(like the more conventional meaning of "tags"). They can however be used, and most
commonly are used, as simple labels.

[ Note: this document uses the word "label" to refer to tags in the more common sense
  of the word - as a named labels like "Birthday", "Work", etc. that are used by users
  in applications to group multiple emails, files etc. based on their shared characteristic.
  The word "tag" in this document always refers to the concept of Akonadi Tags. ]


Akonadi Tags have an ID, type, GID, RemoteID and a parent.

* ID - an immutable unique identifier within Akonadi instance, same as Item and Collection IDs
* Type - an immutable string describing the type of a Tag. Applications can define custom tag
  types and can fetch Tags of only a certain type.
* GID - an immutable string-based identifier. GID can be used to lookup a Tag in Akonadi. If
  application needs to store a reference to a Tag in a configuration file or a cache it should
  use GID rather than ID. GID may or may not be a human-readable string. Generally for Tags
  exposed to UI one should use Akonadi::Tag::setName() to set a human-readable name of the Tag
  (Akonadi::Tag::setName() is equivalent to adding Akonadi::TagAttribute and setting its displayName).
* RemoteID - an identifier used by Akonadi Resources to identify the Tag in the remote storage.
* Parent - Akonadi allows for hierarchical Tags. The semantics of the hierarchy are only meaningful
  to the application or the user, Akonadi does not make any assumptions about the meaning of the
  hierarchy.
* Attributes - same as Item or Collection attributes

# Tag type # {#tag_types} 

Akonadi Tags have types to differentiate between different usecases of Tags.
Applications can define their own types of Tags - for example an email client
could create a Tag of type SENDER, tagging each email with Rags representing
the email sender, allowing to quickly perform a reverse-lookup for all emails
from a certain sender.

In Akonadi, two Tag types are present by default: PLAIN and GENERIC.

There's no limitation as to which attributes can be used with which type. Generally
PLAIN Tags are not used much, and the GENERIC Tags are used as labels.


# Tag GID # {#tag_gid}

For the needs of uniquely identifying Tags, each Tag has a GID. The GID is immutable
and should be used by applications if they need to store a reference to a Tag in
a configuration file or a cache.

GID is also used in Tag merging: when a Resource tries to create a new Tag with a
GID that already exists in Akonadi, the Tags are assumed to be the same and the
new Tag is merged into the existing Tag (see Tag Remote ID below for details and
example as how Tags are handled from Resource point of view).



# Tag RemoteID # {#tag_rid}

Much like with Items and Collections remote IDs, Tag RIDs are used by Akonadi Agents
and Resources to be able to map the Akonadi Tag to their representation in the
backend storage (IMAP server, CalDAV server, etc.)

There is one major difference between RIDs for Items or Collections and for Tags.
Unlike Items and Collections which all have a strictly defined owner Resource,
Tags don't have an owner. This is because Tags can group entities belonging to
different Resources. For this reason, each Tag has multiple remote IDs, where
each Remote ID belongs to an individual Resource. The isolation of the Remote IDs
is fully handled by the server and is opaque to the Resources.

For this reason, Tag Remote IDs are not exposed to client applications, instead
client applications should use the GID, with is immutable and shared between
both clients and Resources.


As an example, let's have a Tag of GENERIC type with GID "birthday":

An IMAP Resource may fetch an Item with a flag "$BIRTHDAY". The resource will then
create a new GENERIC Tag with GID "birthday" and remoteID "$BIRTHDAY". The
Akonadi Server, instead of creating another Tag with GID "birthday" instead
adds the "$BIRTHDAY" remoteID with relation to the IMAP resource to the existing
"birthday" Tag.

A DAV Resource may fetch an Item with a label "dav:birthday". The resource will
then create a new GENERIC Tag with GID "birthday" and remoteID "dav:birthday".
The Akonadi Server will add the "dav:birthday" and the relation to the DAV resource
to the existing "birthday" Tagg as well.

When user decides to add the "birthday" tag to two  events - one owned by the DAV
resource and one owned by the IMAP resource the Akonadi server will send a change
notification to the DAV Resource with the "dav:birthday" RID and with the "$BIRTHDAY"
RID to the IMAP Resource.


# Client API # {#tag_client_api}

Existing Tags can be retrieved from Akonadi using Akonadi::TagFetchJob.

To create a new GENERIC Tag with TagAttribute with display name set, one should use
Akonadi::Tag::genericTag(const QString &name) static method. The tag will have a
randomly generated GID set. This is equivalent to
calling

~~~~~~~~~~~~~{.cpp}
Tag tag;
tag.setType("GENERIC");
tag.setGid(QUuid::createUuid().toByteArray());
tag.setName(name);
~~~~~~~~~~~~~


Such Tag can now be uploaded to Akonadi using Akonadi::TagCreateJob. If a Tag with
the same Type and GID already exist in Akonadi, all other data in the existing
Tag will be overwritten by data in the new tag.

Any changes to a Tag can be stored in Akonaddi using Akonadi::TagModifyJob. Resources
can use Akonadi::TagModifyJob to set or unset a RemoteID for an existing Tag. If the
Tag has no Remote IDs left after this, the Tag will automatically be removed from
Akonadi.

A tag can be removed from Akonadi using Akonadi::TagDeleteJob. Removing a Tag
will also untag it from all Items and will notify all Resources about it so
that the entities are untagged in their backend storage as well.

To retrieve all Items with a certain Tag, Akonadi::ItemFetchJob has an overloaded
constructor that takes an Akonadi::Tag as an argument.

Tags for each Item are available in Akonadi::Item::tags(). To tag or untag an Item
modify the list of Tags in the Item and store the change in Akonadi using
Akonadi::ItemModifyJob.


## Fetch Scope ## {#tag_fetch_scope}

By default Item Tags are not fetched when simply running Akonadi::ItemFetchJob. To
enable fetching Tags, you have to set Akonadi::ItemFetchScope::fetchTags to true.
The retrieved Tags will only have their ID set. To retrieve more you need to modify
the Akonadi::TagFetchScope (from Akonadi::ItemFetchScope::fetchScope()) and unset
Akonadi::TagFetchScope::fetchIdOnly(). Now ItemFetchJob will return Items with Tags
with ID, GID. If the Akonadi::TagFetchScope::attributes() is empty then all attributes
will be received. Otherwise you can restrict the fetch scope to only fetch certain
attributes.


## Implementation in Resources ## {#tag_resource_impl}

If the storage backend supports some form of labels it should implement support for Tags
so that tags from the backend are synchronized into Akonadi and vice versa.

To be notified about changes in Tags, Resource implementation must inherit from
Akonadi::AgentBase::ObserverV4 (or later). 

Akonadi::AgentBase::ObserverV4::tagAdded(), tagChanged() and tagRemoved() may be
optionally reimplemented by Resource implementation if it wants to synchronize
all local Tags with the remote storage so they can be presented to the user, even
if no Items belonging to the Resource are tagged with it.

The most important method to reimplement by Resource implementations is
Akonadi::AgentBase::ObserverV4::itemsTagsChanged(), which gives a list of changed
Items belonging to the Resource, a set of Tags added to all those Items and a set
of Tags removed from those Items.

