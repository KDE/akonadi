INSERT INTO CachePolicies (name) VALUES ('none');
INSERT INTO CachePolicies (name) VALUES ('temporary');
INSERT INTO CachePolicies (name) VALUES ('permanent');

INSERT INTO MimeTypes (mime_type) VALUES ('message/rfc822');
INSERT INTO MimeTypes (mime_type) VALUES ('text/ical');
INSERT INTO MimeTypes (mime_type) VALUES ('text/vcard');
INSERT INTO MimeTypes (mime_type) VALUES ('directory/inode');

INSERT INTO MetaTypes SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/ical';
INSERT INTO MetaTypes SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/ical';
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
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 3, 4);

INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (0, 'testmailbody', 0, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (1, 'testmailbody1', 0, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (2, 'testmailbody2', 0, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (3, 'testmailbody3', 1, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (4, 'testmailbody4', 2, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (5, 'testmailbody5', 2, 0);
INSERT INTO PimItems (id, data, location_id, mimetype_id) VALUES (6, 'testmailbody6', 1, 0);

INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (6, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (4, 1);
