
CREATE TABLE MessageBoxType (
    id                  smallint PRIMARY KEY,
    name                text UNIQUE
);

INSERT INTO MessageBoxType(id, name) VALUES
    (1, 'Mail Box'),
    (2, 'Forum'),
    (3, 'Wiki Category');


--
CREATE TABLE MessageBox (
    id                  bigint PRIMARY KEY,
    parent              bigint NULL REFERENCES(Forum),
    message_box_type    smallint REFERENCES(MessageBoxType),
    name                text,
    description         text,
    owner               bigint REFERENCES(Entity),

    UNIQUE KEY (parent, name)
);

-- A message can be a:
--  * Mail send from one entity to another entity. Mail gets
--    directly deposited in the INBOX of the recipient.
--  * Forum post from one entity into a forum that the sener
--    has permission to post in.
--  * Wiki page from one entity and multiple editors.
--
-- Mails and forum post may be in reply to another message.
--
CREATE TABLE Message (
    id                  bigint PRIMARY KEY,
    contained_by        bigint[], -- REFERENCES(MessageBox)
    visible             boolean

    in_reply_to         bigint NULL REFERENCES(Message),
    send_time           datetime,
    sender              bigint REFERENCES(Entity)
    recipients          bigint[] -- REFERENCES(Entity)
);

-- A message can be edited by the person who created the original message
-- or by a moderator. For storage reasons we store only the delta between
-- each version.
--
-- The message text itself is a local version of markdown which allows linking
-- of game items and entities.
--
-- The message delta also includes information by whom and when the message
-- was edited.
CREATE TABLE MessageDelta (
    message             bigint REFERENCES(Message),
    version             int,
    subject             text,
    message_delta       text,

    edited_by           bigint REFERENCES(Entity),
    edited_at           datetime

    PRIMARY KEY (message, version)
)
