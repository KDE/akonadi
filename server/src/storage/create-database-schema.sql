CREATE TABLE CachePolicies
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	name		CHAR(20) NOT NULL UNIQUE
	
);

CREATE TABLE Resources
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	name		CHAR(60) NOT NULL UNIQUE,
	cachepolicy_id	INTEGER NOT NULL REFERENCES CachePolicies(id)
);

CREATE TABLE Locations
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	uri		CHAR(512) NOT NULL UNIQUE,
	cachepolicy_id	INTEGER REFERENCES CachePolicies(id),
	resource_id	INTEGER NOT NULL REFERENCES Resources(id),
	exists_count    INTEGER,
	recent_count    INTEGER,
	unseen_count    INTEGER,
	first_unseen    INTEGER,
	uid_validity    INTEGER
);

CREATE TABLE PersistentSearches
(
  id    INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  name  CHAR(512) NOT NULL UNIQUE,
  query CHAR(512) NOT NULL
);

CREATE TABLE MimeTypes
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	mime_type	CHAR(60) NOT NULL UNIQUE
);

CREATE TABLE MetaTypes
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	name		CHAR(60) NOT NULL,
	mimetype_id	INTEGER REFERENCES MimeTypes(id)
);

CREATE TABLE PimItems
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
-- IMAP flags can be put here
-- "data" might be BLOB as well, or we can store the data in parts
	data		TEXT,
	location_id	INTEGER REFERENCES Locations(id),
	mimetype_id	INTEGER REFERENCES MimeTypes(id)
);

CREATE TABLE ItemMetaData
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	data		CHAR(512) NOT NULL,
	metatype_id	INTEGER REFERENCES MetaTypes(id),
	pimitem_id	INTEGER REFERENCES PimItems(id)
);

CREATE TABLE Flags
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	name		CHAR(20) NOT NULL UNIQUE
);

CREATE TABLE ItemFlags
(
	flag_id		INTEGER REFERENCES Flags(id),
	pimitem_id	INTEGER REFERENCES PimItems(id)
);

CREATE TABLE Parts
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
-- "data" might be BLOB as well
	data		TEXT,
	datasize	INTEGER NOT NULL,
	pimitem_id	INTEGER REFERENCES PimItems(id),
	mimetype_id	INTEGER REFERENCES MimeTypes(id)
);

CREATE TABLE PartMetaData
(
	id		INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	data		CHAR(512) NOT NULL,
	metatype_id	INTEGER REFERENCES MetaTypes(id),
	part_id		INTEGER REFERENCES Parts(id)
);

CREATE TABLE PartFlags
(
	flag_id		INTEGER REFERENCES Flags(id),
	part_id		INTEGER REFERENCES Parts(id)
);

CREATE TABLE LocationMimeTypes
(
        location_id		INTEGER REFERENCES Locations(id),
	mimetype_id		INTEGER REFERENCES MimeTypes(id)
);

