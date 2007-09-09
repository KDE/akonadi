DELETE FROM PimItemFlagRelation;
DELETE FROM PimItemTable;
DELETE FROM PartTable;
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

INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'kde-core-devel', '<request><userQuery>kde-core-devel@kde.org</userQuery></request>', 1);
INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'all', '<request><userQuery>MIMETYPE message/rfc822</userQuery></request>', 1);
INSERT INTO LocationTable (parentId, name, remoteId, resourceId) VALUES(1, 'Test ?er', '<request><userQuery>"Test ?er"</userQuery></request>', 1);

INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 3);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 4);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 2);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 2, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 10, 1);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 3, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 4, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 5);
INSERT INTO LocationMimeTypeRelation ( Location_id, MimeType_id) VALUES( 8, 1);

INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (1, 'A', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (2, 'B', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (3, 'C', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (4, 'D', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (5, 'E', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (6, 'F', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (7, 'G', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (8, 'H', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (9, 'I', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (10, 'J', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (11, 'K', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (12, 'L', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (13, 'M', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (14, 'N', 10, 1);
INSERT INTO PimItemTable (id, remoteId, locationId, mimeTypeId) VALUES (15, 'O', 10, 1);

INSERT INTO PartTable (pimItemId, name, data) VALUES ( 1, 'RFC822', 'testmailbody' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 2, 'RFC822','testmailbody1' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 3, 'RFC822','testmailbody2' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 4, 'RFC822','testmailbody3' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 5, 'RFC822','testmailbody4' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 6, 'RFC822','testmailbody5' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 7, 'RFC822','testmailbody6' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 8, 'RFC822','testmailbody7' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 9, 'RFC822','testmailbody8' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 10, 'RFC822','testmailbody9' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 11, 'RFC822','testmailbody10' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 12, 'RFC822','testmailbody11' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 13, 'RFC822','testmailbody12' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 14, 'RFC822','testmailbody13' );
INSERT INTO PartTable (pimItemId, name, data) VALUES ( 15, 'RFC822','testmailbody14' );

INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (8, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (7, 1);
INSERT INTO PimItemFlagRelation (Flag_id, PimItem_id) VALUES (5, 2);
