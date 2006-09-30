DELETE FROM MetaTypes;
-- INSERT INTO MetaTypes SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypes SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypes SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypes SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypes SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

DELETE FROM ItemFlags;
DELETE FROM PimItems;
DELETE FROM LocationMimeTypes;
DELETE FROM Locations;
DELETE FROM Resources;

INSERT INTO Resources (id, name, cachepolicy_id) VALUES(1, 'akonadi_dummy_resource_1', 1);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(2, 'akonadi_dummy_resource_2', 1);
INSERT INTO Resources (id, name, cachepolicy_id) VALUES(3, 'akonadi_dummy_resource_3', 1);

INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (1, 'res1/foo', 1, 1, 3, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (2, 'res1/foo/bar', 1, 1, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (3, 'res1/foo/bar/bla', 1, 1, 2, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (4, 'res1/foo/bla', 1, 1, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (5, 'res2/foo2', 1, 2, 2, 3, 4, 5, 6);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (6, 'res1', 1, 1, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (7, 'res2', 1, 2, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (8, 'res3', 1, 3, 0, 0, 0, 0, 0);
INSERT INTO Locations (id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity) VALUES (9, 'res2/space folder', 1, 2, 0, 0, 0, 0, 0);

DELETE FROM PersistentSearches;
INSERT INTO PersistentSearches (id, name, query) VALUES(0, 'kde-core-devel', 'MIMETYPE message/rfc822 HEADER From kde-core-devel@kde.org');
INSERT INTO PersistentSearches (id, name, query) VALUES(1, 'all', 'MIMETYPE message/rfc822 ALL');
INSERT INTO PersistentSearches (id, name, query) VALUES(2, 'Test ?er', 'MIMETYPE message/rfc822 BODY "Test ?er"');

INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 2);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 3);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 1, 1);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 3, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 4, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 8, 4);
INSERT INTO LocationMimeTypes ( location_id, mimetype_id) VALUES( 8, 1);

INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (1, 'A', 'testmailbody', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (2, 'B', 'testmailbody1', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (3, 'C', 'testmailbody2', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (4, 'D', 'testmailbody3', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (5, 'E', 'testmailbody4', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (6, 'F', 'testmailbody5', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (7, 'G', 'testmailbody6', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (8, 'H', 'testmailbody7', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (9, 'I', 'testmailbody8', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (10, 'J', 'testmailbody9', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (11, 'K', 'testmailbody10', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (12, 'L', 'testmailbody11', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (13, 'M', 'testmailbody12', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (14, 'N', 'testmailbody13', 1, 1);
INSERT INTO PimItems (id, remote_id, data, location_id, mimetype_id) VALUES (15, 'O', 'testmailbody14', 1, 1);

INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (8, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (7, 1);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 2);
INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (5, 2);

DELETE FROM ItemMetaData;
-- INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (0, 'testmetatata', 0, 0);
-- INSERT INTO ItemMetaData (id, data, metatype_id, pimitem_id) VALUES (1, 'testmetatata2', 0, 3);
