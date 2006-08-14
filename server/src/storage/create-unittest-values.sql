DELETE FROM MetaTypes;
INSERT INTO MetaTypes SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
INSERT INTO MetaTypes SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
INSERT INTO MetaTypes SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

DELETE FROM Locations;
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (0, 'res1/foo', 1, 0, 3, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (1, 'res1/foo/bar', 1, 0, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (2, 'res1/foo/bar/bla', 1, 0, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (3, 'res1/foo/bla', 1, 0, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (4, 'res2/foo2', 1, 1, 2, 3, 4, 5, 6);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (5, 'res1', 1, 0, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (6, 'res2', 1, 1, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (7, 'res3', 1, 2, 0, 0, 0, 0, 0);

DELETE FROM Resources;
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(0, 'akonadi_dummy_resource_1', 1);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(1, 'akonadi_dummy_resource_2', 1);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(2, 'akonadi_dummy_resource_3', 1);

DELETE FROM PersistentSearches;
INSERT INTO PersistentSearches (id, name, query) VALUES(0, 'kde-core-devel', 'MIMETYPE message/rfc822 HEADER From kde-core-devel@kde.org');
INSERT INTO PersistentSearches (id, name, query) VALUES(1, 'all', 'MIMETYPE message/rfc822 ALL');
INSERT INTO PersistentSearches (id, name, query) VALUES(2, 'Test �er', 'MIMETYPE message/rfc822 BODY "Test �er"');

DELETE FROM LocationMimeTypes;
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 2);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 3);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 0, 1);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 3, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 7, 4);

DELETE FROM PimItems;
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (0, 'A', 'testmailbody', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (1, 'B', 'testmailbody1', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (2, 'C', 'testmailbody2', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (3, 'D', 'testmailbody3', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (4, 'E', 'testmailbody4', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (5, 'F', 'testmailbody5', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (6, 'G', 'testmailbody6', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (7, 'H', 'testmailbody7', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (8, 'I', 'testmailbody8', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (9, 'J', 'testmailbody9', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (10, 'K', 'testmailbody10', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (11, 'L', 'testmailbody11', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (12, 'M', 'testmailbody12', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (13, 'N', 'testmailbody13', 0, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (14, 'O', 'testmailbody14', 0, 1);

DELETE FROM ItemFlags;
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (6, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (7, 0);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 1);

DELETE FROM ItemMetaData;
INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (0, "testmetatata", 0, 0);
INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (1, "testmetatata2", 0, 3);
