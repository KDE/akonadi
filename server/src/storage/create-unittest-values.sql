DELETE FROM MetaTypeTable;
-- INSERT INTO MetaTypeTable SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypeTable SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypeTable SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypeTable SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypeTable SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

DELETE FROM PimItemFlagRelation;
DELETE FROM PimItemTable;
DELETE FROM LocationMimeTypeRelation;
DELETE FROM LocationTable;
DELETE FROM ResourceTable;

INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(1, 'akonadi_dummy_resource_1', 1);
INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(2, 'akonadi_dummy_resource_2', 1);
INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(3, 'akonadi_dummy_resource_3', 1);

INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (1, 'res1/foo', 1, 1, 3, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (2, 'res1/foo/bar', 1, 1, 2, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (3, 'res1/foo/bar/bla', 1, 1, 2, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (4, 'res1/foo/bla', 1, 1, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (5, 'res2/foo2', 1, 2, 2, 3, 4, 5, 6);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (6, 'res1', 1, 1, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (7, 'res2', 1, 2, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (8, 'res3', 1, 3, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (9, 'res2/space folder', 1, 2, 0, 0, 0, 0, 0);

DELETE FROM PersistentSearchTable;
INSERT INTO PersistentSearchTable (id, name, query) VALUES(1, 'kde-core-devel', 'MIMETYPE message/rfc822 HEADER From kde-core-devel@kde.org');
INSERT INTO PersistentSearchTable (id, name, query) VALUES(2, 'all', 'MIMETYPE message/rfc822 ALL');
INSERT INTO PersistentSearchTable (id, name, query) VALUES(3, 'Test ?er', 'MIMETYPE message/rfc822 BODY "Test ?er"');

INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 1, 2);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 1, 3);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 1, 1);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 2, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 3, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 4, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 1);

INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (1, 'A', 'testmailbody', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (2, 'B', 'testmailbody1', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (3, 'C', 'testmailbody2', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (4, 'D', 'testmailbody3', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (5, 'E', 'testmailbody4', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (6, 'F', 'testmailbody5', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (7, 'G', 'testmailbody6', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (8, 'H', 'testmailbody7', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (9, 'I', 'testmailbody8', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (10, 'J', 'testmailbody9', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (11, 'K', 'testmailbody10', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (12, 'L', 'testmailbody11', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (13, 'M', 'testmailbody12', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (14, 'N', 'testmailbody13', 1, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (15, 'O', 'testmailbody14', 1, 1);

INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (8, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (7, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 2);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 2);

DELETE FROM ItemMetaDataTable;
-- INSERT INTO ItemMetaData (id, data, metatype_id, PimItem_id) VALUES (0, 'testmetatata', 0, 0);
-- INSERT INTO ItemMetaData (id, data, metatype_id, PimItem_id) VALUES (1, 'testmetatata2', 0, 3);
