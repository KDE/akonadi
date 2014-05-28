/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_PROTOCOL_P_H
#define AKONADI_PROTOCOL_P_H

/**
  @file protocol_p.h Shared constants used in the communication protocol between
  the Akonadi server and its clients.

  @todo Fill this file with command names, item/collection property names
  item part names, etc. and replace the usages accordingly.
*/

// D-Bus service names
#define AKONADI_DBUS_SERVER_SERVICE           "org.freedesktop.Akonadi"
#define AKONADI_DBUS_CONTROL_SERVICE          "org.freedesktop.Akonadi.Control"
#define AKONADI_DBUS_CONTROL_SERVICE_LOCK     "org.freedesktop.Akonadi.Control.lock"
#define AKONADI_DBUS_AGENTSERVER_SERVICE      "org.freedesktop.Akonadi.AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_SERVICE   "org.freedesktop.Akonadi.Janitor"
#define AKONADI_DBUS_SERVER_SERVICE_UPGRADING "org.freedesktop.Akonadi.upgrading"

#define AKONADI_DBUS_AGENTMANAGER_PATH   "/AgentManager"
#define AKONADI_DBUS_AGENTSERVER_PATH    "/AgentServer"
#define AKONADI_DBUS_STORAGEJANITOR_PATH "/Janitor"

// Commands
#define AKONADI_CMD_APPEND           "APPEND"
#define AKONADI_CMD_BEGIN            "BEGIN"
#define AKONADI_CMD_CAPABILITY       "CAPABILITY"
#define AKONADI_CMD_COLLECTION       "COLLECTION"
#define AKONADI_CMD_COLLECTIONCOPY   "COLCOPY"
#define AKONADI_CMD_COLLECTIONMOVE   "COLMOVE"
#define AKONADI_CMD_COMMIT           "COMMIT"
#define AKONADI_CMD_ITEMCOPY         "COPY"
#define AKONADI_CMD_COLLECTIONCREATE "CREATE"
#define AKONADI_CMD_COLLECTIONDELETE "DELETE"
#define AKONADI_CMD_EXPUNGE          "EXPUNGE"
#define AKONADI_CMD_ITEMFETCH        "FETCH"
#define AKONADI_CMD_GID              "GID"
#define AKONADI_CMD_HRID             "HRID"
#define AKONADI_CMD_ITEMLINK         "LINK"
#define AKONADI_CMD_LIST             "LIST"
#define AKONADI_CMD_LOGIN            "LOGIN"
#define AKONADI_CMD_LOGOUT           "LOGOUT"
#define AKONADI_CMD_LSUB             "LSUB"
#define AKONADI_CMD_MERGE            "MERGE"
#define AKONADI_CMD_COLLECTIONMODIFY "MODIFY"
#define AKONADI_CMD_ITEMMOVE         "MOVE"
#define AKONADI_CMD_ITEMDELETE       "REMOVE"
#define AKONADI_CMD_RESOURCESELECT   "RESSELECT"
#define AKONADI_CMD_RID              "RID"
#define AKONADI_CMD_ROLLBACK         "ROLLBACK"
#define AKONADI_CMD_SUBSCRIBE        "SUBSCRIBE"
#define AKONADI_CMD_SEARCH           "SEARCH"
#define AKONADI_CMD_SEARCH_RESULT    "SEARCH_RESULT"
#define AKONADI_CMD_SEARCH_STORE     "SEARCH_STORE"
#define AKONADI_CMD_SELECT           "SELECT"
#define AKONADI_CMD_STATUS           "STATUS"
#define AKONADI_CMD_ITEMMODIFY       "STORE"
#define AKONADI_CMD_TAGAPPEND        "TAGAPPEND"
#define AKONADI_CMD_TAGFETCH         "TAGFETCH"
#define AKONADI_CMD_TAGREMOVE        "TAGREMOVE"
#define AKONADI_CMD_TAGSTORE         "TAGSTORE"
#define AKONADI_CMD_UID              "UID"
#define AKONADI_CMD_ITEMUNLINK       "UNLINK"
#define AKONADI_CMD_UNSUBSCRIBE      "UNSUBSCRIBE"
#define AKONADI_CMD_ITEMCREATE       "X-AKAPPEND"
#define AKONADI_CMD_X_AKLIST         "X-AKLIST"
#define AKONADI_CMD_X_AKLSUB         "X-AKLSUB"

// Command parameters
#define AKONADI_PARAM_CAPABILITY_AKAPPENDSTREAMING "AKAPPENDSTREAMING"
#define AKONADI_PARAM_ALLATTRIBUTES                "ALLATTR"
#define AKONADI_PARAM_ANCESTORS                    "ANCESTORS"
#define AKONADI_PARAM_ATR                          "ATR:"
#define AKONADI_PARAM_CACHEONLY                    "CACHEONLY"
#define AKONADI_PARAM_CACHEDPARTS                  "CACHEDPARTS"
#define AKONADI_PARAM_CACHETIMEOUT                 "CACHETIMEOUT"
#define AKONADI_PARAM_CACHEPOLICY                  "CACHEPOLICY"
#define AKONADI_PARAM_CHANGEDSINCE                 "CHANGEDSINCE"
#define AKONADI_PARAM_CHARSET                      "CHARSET"
#define AKONADI_PARAM_CHECKCACHEDPARTSONLY         "CHECKCACHEDPARTSONLY"
#define AKONADI_PARAM_COLLECTION                   "COLLECTION"
#define AKONADI_PARAM_COLLECTIONID                 "COLLECTIONID"
#define AKONADI_PARAM_COLLECTIONS                  "COLLECTIONS"
#define AKONADI_PARAM_MTIME                        "DATETIME"
#define AKONADI_PARAM_CAPABILITY_DIRECTSTREAMING   "DIRECTSTREAMING"
#define AKONADI_PARAM_UNDIRTY                      "DIRTY"
#define AKONADI_PARAM_DISPLAY                      "DISPLAY"
#define AKONADI_PARAM_EXTERNALPAYLOAD              "EXTERNALPAYLOAD"
#define AKONADI_PARAM_ENABLED                      "ENABLED"
#define AKONADI_PARAM_FLAGS                        "FLAGS"
#define AKONADI_PARAM_TAGS                         "TAGS"
#define AKONADI_PARAM_FULLPAYLOAD                  "FULLPAYLOAD"
#define AKONADI_PARAM_GID                          "GID"
#define AKONADI_PARAM_IGNOREERRORS                 "IGNOREERRORS"
#define AKONADI_PARAM_INDEX                        "INDEX"
#define AKONADI_PARAM_INHERIT                      "INHERIT"
#define AKONADI_PARAM_INTERVAL                     "INTERVAL"
#define AKONADI_PARAM_INVALIDATECACHE              "INVALIDATECACHE"
#define AKONADI_PARAM_MIMETYPE                     "MIMETYPE"
#define AKONADI_PARAM_MERGE                        "MERGE"
#define AKONADI_PARAM_LOCALPARTS                   "LOCALPARTS"
#define AKONADI_PARAM_NAME                         "NAME"
#define AKONADI_PARAM_CAPABILITY_NOTIFY            "NOTIFY"
#define AKONADI_PARAM_CAPABILITY_NOPAYLOADPATH     "NOPAYLOADPATH"
#define AKONADI_PARAM_PARENT                       "PARENT"
#define AKONADI_PARAM_PERSISTENTSEARCH             "PERSISTENTSEARCH"
#define AKONADI_PARAM_PARTS                        "PARTS"
#define AKONADI_PARAM_PLD                          "PLD:"
#define AKONADI_PARAM_PLD_RFC822                   "PLD:RFC822"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYCOLLECTIONS "QUERYCOLLECTIONS"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG   "QUERYLANGUAGE"
#define AKONADI_PARAM_PERSISTENTSEARCH_QUERYSTRING "QUERYSTRING"
#define AKONADI_PARAM_QUERY                        "QUERY"
#define AKONADI_PARAM_RECURSIVE                    "RECURSIVE"
#define AKONADI_PARAM_REMOTE                       "REMOTE"
#define AKONADI_PARAM_REMOTEID                     "REMOTEID"
#define AKONADI_PARAM_REMOTEREVISION               "REMOTEREVISION"
#define AKONADI_PARAM_RESOURCE                     "RESOURCE"
#define AKONADI_PARAM_REVISION                     "REV"
#define AKONADI_PARAM_SILENT                       "SILENT"
#define AKONADI_PARAM_DOT_SILENT                   ".SILENT"
#define AKONADI_PARAM_CAPABILITY_SERVERSEARCH      "SERVERSEARCH"
#define AKONADI_PARAM_SIZE                         "SIZE"
#define AKONADI_PARAM_STATISTICS                   "STATISTICS"
#define AKONADI_PARAM_SYNC                         "SYNC"
#define AKONADI_PARAM_SYNCONDEMAND                 "SYNCONDEMAND"
#define AKONADI_PARAM_TAG                          "TAG"
#define AKONADI_PARAM_TAGID                        "TAGID"
#define AKONADI_PARAM_UID                          "UID"
#define AKONADI_PARAM_VIRTUAL                      "VIRTUAL"

// Flags
#define AKONADI_FLAG_GID                           "\\Gid"
#define AKONADI_FLAG_IGNORED                       "$IGNORED"
#define AKONADI_FLAG_MIMETYPE                      "\\MimeType"
#define AKONADI_FLAG_REMOTEID                      "\\RemoteId"
#define AKONADI_FLAG_REMOTEREVISION                "\\RemoteRevision"
#define AKONADI_FLAG_TAG                           "\\Tag"
#define AKONADI_FLAG_RTAG                          "\\RTag"
#define AKONADI_FLAG_SEEN                          "\\SEEN"

// Attributes
#define AKONADI_ATTRIBUTE_HIDDEN                   "ATR:HIDDEN"
#define AKONADI_ATTRIBUTE_MESSAGES                 "MESSAGES"
#define AKONADI_ATTRIBUTE_UNSEEN                   "UNSEEN"

// special resource names
#define AKONADI_SEARCH_RESOURCE                    "akonadi_search_resource"
#endif
