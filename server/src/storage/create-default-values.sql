INSERT INTO CachePolicies (name) VALUES ('none');
INSERT INTO CachePolicies (name) VALUES ('temporary');
INSERT INTO CachePolicies (name) VALUES ('permanent');

INSERT INTO MimeTypes (id,mime_type) VALUES (0,'message/rfc822');
INSERT INTO MimeTypes (id,mime_type) VALUES (1,'text/calendar');
INSERT INTO MimeTypes (id,mime_type) VALUES (2,'text/vcard');
INSERT INTO MimeTypes (id,mime_type) VALUES (3,'inode/directory');

INSERT INTO MetaTypes SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
INSERT INTO MetaTypes SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
INSERT INTO MetaTypes SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

INSERT INTO Flags (name) VALUES ('important');
INSERT INTO Flags (name) VALUES ('has_attachment');
INSERT INTO Flags (name) VALUES ('spam');
INSERT INTO Flags (name) VALUES ('\Answered');
INSERT INTO Flags (name) VALUES ('\Flagged');
INSERT INTO Flags (name) VALUES ('\Deleted');
INSERT INTO Flags (name) VALUES ('\Seen');
INSERT INTO Flags (name) VALUES ('\Draft');

INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (0, '/foo', 0, 0, 3, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (1, '/foo/bar', 0, 0, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (2, '/foo/bar/bla', 0, 0, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (3, '/foo/bla', 0, 0, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (4, '/foo2', 0, 1, 2, 3, 4, 5, 6);

INSERT INTO Resources (id, name, cachepolicy_id) VALUES(0, 'res1', 0);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(1, 'res2', 0);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(2, 'res3', 0);

INSERT INTO PersistentSearches (id, name, query) VALUES(0, 'kde-core-devel', 'MIMETYPE message/rfc822 HEADER From kde-core-devel@kde.org');
INSERT INTO PersistentSearches (id, name, query) VALUES(1, 'all', 'MIMETYPE message/rfc822 ALL');
INSERT INTO PersistentSearches (id, name, query) VALUES(2, 'Test �er', 'MIMETYPE message/rfc822 BODY "Test �er"');

INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 1);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 2);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 2);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 0);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 3);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 2, 3);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 3, 3);

INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (0, 'A', 'testmailbody', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (1, 'B', 'testmailbody1', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (2, 'C', 'testmailbody2', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (3, 'D', 'testmailbody3', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (4, 'E', 'testmailbody4', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (5, 'F', 'testmailbody5', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (6, 'G', 'testmailbody6', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (7, 'H', 'testmailbody7', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (8, 'I', 'testmailbody8', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (9, 'J', 'testmailbody9', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (10, 'K', 'testmailbody10', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (11, 'L', 'testmailbody11', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (12, 'M', 'testmailbody12', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (13, 'N', 'testmailbody13', 0, 0);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (14, 'O', 'testmailbody14', 0, 0);

INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (6, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 1);

INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (0, "testmetatata", 0, 0);
INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (1, "testmetatata2", 0, 3);
