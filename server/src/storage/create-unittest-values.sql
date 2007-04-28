DELETE FROM MetaTypeTable;
-- INSERT INTO MetaTypeTable SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypeTable SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
-- INSERT INTO MetaTypeTable SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypeTable SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/calendar';
-- INSERT INTO MetaTypeTable SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

DELETE FROM PimItemFlagRelation;
DELETE FROM PimItemTable;
DELETE FROM LocationMimeTypeRelation;
DELETE FROM LocationTable WHERE name != "Search" or parentId != 0;
DELETE FROM ResourceTable WHERE name != "akonadi_search_resource";

INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(2, 'akonadi_dummy_resource_1', 1);
INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(3, 'akonadi_dummy_resource_2', 1);
INSERT INTO ResourceTable (id, name, cachePolicyId) VALUES(4, 'akonadi_dummy_resource_3', 1);

INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (10, 6, 'foo', 3, 2, 3, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (2, 10, 'bar', 1, 2, 2, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (3, 2, 'bla', 1, 2, 2, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (4, 10, 'bla', 1, 2, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (5, 7, 'foo2', 1, 3, 2, 3, 4, 5, 6);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (6, 0, 'res1', 1, 2, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (7, 0, 'res2', 1, 3, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (8, 0, 'res3', 1, 4, 0, 0, 0, 0, 0);
INSERT INTO LocationTable (id, parentId, name, cachePolicyId, resourceId, existCount, recentCount, unseenCount, firstUnseen, uidValidity) VALUES (9, 7, 'space folder', 1, 3, 0, 0, 0, 0, 0);

INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'kde-core-devel', 'MIMETYPE message/rfc822 HEADER From kde-core-devel@kde.org', 1);
INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'all', 'MIMETYPE message/rfc822 ALL', 1);
INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'Test ?er', 'MIMETYPE message/rfc822 BODY "Test ?er"', 1);

INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 3);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 2);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 2, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 1);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 2, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 3, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 4, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 1);

INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (1, 'A', 'testmailbody', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (2, 'B', 'testmailbody1', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (3, 'C', 'testmailbody2', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (4, 'D', 'testmailbody3', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (5, 'E', 'testmailbody4', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (6, 'F', 'testmailbody5', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (7, 'G', 'testmailbody6', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (8, 'H', 'testmailbody7', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (9, 'I', 'testmailbody8', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (10, 'J', 'testmailbody9', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (11, 'K', 'testmailbody10', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (12, 'L', 'testmailbody11', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (13, 'M', 'testmailbody12', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (14, 'N', 'testmailbody13', 10, 1);
INSERT INTO PimItemTable (id, remoteId, data, locationId, mimeTypeId) VALUES (15, 'O', 'testmailbody14', 10, 1);

INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (8, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (7, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 2);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 2);

DELETE FROM ItemMetaDataTable;
-- INSERT INTO ItemMetaData (id, data, metatype_id, PimItem_id) VALUES (0, 'testmetatata', 0, 0);
-- INSERT INTO ItemMetaData (id, data, metatype_id, PimItem_id) VALUES (1, 'testmetatata2', 0, 3);
