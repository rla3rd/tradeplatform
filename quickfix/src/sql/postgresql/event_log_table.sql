CREATE SEQUENCE event_log_sequence;

CREATE TABLE event_log (
  id INTEGER DEFAULT NEXTVAL('event_log_sequence'),
  time TIMESTAMP NOT NULL,
  beginstring CHAR(8),
  sendercompid VARCHAR(64),
  targetcompid VARCHAR(64),
  session_qualifier VARCHAR(64),
  text TEXT NOT NULL,
  PRIMARY KEY (id)
);