INSERT INTO CachePolicies (name) VALUES ('none');
INSERT INTO CachePolicies (name) VALUES ('temporary');
INSERT INTO CachePolicies (name) VALUES ('permanent');

INSERT INTO MimeTypes (mime_type) VALUES ('message/rfc822');
INSERT INTO MimeTypes (mime_type) VALUES ('text/ical');
INSERT INTO MimeTypes (mime_type) VALUES ('text/vcard');

INSERT INTO MetaTypes SELECT NULL, 'from', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'to', id FROM MimeTypes WHERE mime_type = 'message/rfc822';
INSERT INTO MetaTypes SELECT NULL, 'start_date', id FROM MimeTypes WHERE mime_type = 'text/ical';
INSERT INTO MetaTypes SELECT NULL, 'end_date', id FROM MimeTypes WHERE mime_type = 'text/ical';
INSERT INTO MetaTypes SELECT NULL, 'email', id FROM MimeTypes WHERE mime_type = 'text/vcard';

INSERT INTO Flags (name) VALUES ('important');
INSERT INTO Flags (name) VALUES ('has_attachment');
INSERT INTO Flags (name) VALUES ('spam');
