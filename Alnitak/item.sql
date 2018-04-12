
-- A property is a 32 bit integer which encodes:
--  * A 16 bit propey type identifier
--  * A 16 bit half-precision float value.

CREATE TABLE PropertyType (
    id                          integer PRIMARY KEY,
    name                        text UNIQUE,
    description                 text
);


CREATE TABLE ItemType (
    id                          integer PRIMARY KEY,
    properties                  bigint[]
);

CREATE TABLE Item (
    id                          bigint PRIMARY KEY,
    nr_items                    bigint NULL,
    contains                    bigint[],       -- This is a reversse-cache of the contained_by relation.
    contained_by                bigint NULL REFERENCES(Item),
    item_type                   integer REFERENCES(ItemType),
    properties                  bigint[],

);

CREATE TABLE ItemTransaction (
    id                          bigint PRIMARY KEY,

    parent_containers           bigint[],       -- Nested transactions are not allowed.
    started_at                  datetime,
    completed_at                datetime NULL
);

CREATE TABLE ItemCommand (
    id                          char PRIMARY KEY,
    name                        text UNIQUE
)

INSERT INTO ItemCommand(id, name) VALUES
    ('m', 'Move'),
    ('d', 'Delete'),
    ('c', 'Create'),
    ('d', 'Drop'),
    ('p', 'Pickup');


CREATE TABLE ItemTransactionLeg (
    item_transaction            bigint REFERENCES(ItemTransaction),
    command                     char REFERENCE(ItemCommand),
    item                        bigint REFERENCES(Item),
    source                      bigint REFERENCES(Item) NULL,
    destination                 bigint REFERENCES(Item) NULL,

    PRIMARY KEY (item_transaction, item)
);

CREATE TABLE ItemTransactionParty (
    item_transaction    bigint REFERENCES(ItemTransaction),
    party               bigint REFERENCES(Entity),
    accept              boolean,

    PRIMARY KEY (item_transaction, party)
);



