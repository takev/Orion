

CREATE TABLE EntityType (
    id                  char PRIMARY KEY,
    name                text
);

INSERT INTO EntityType (id, name) VALUES
    ('p', 'Player Character'),
    ('n', 'Non-player Character'),
    ('g', 'Guild'),
    ('a', 'Aliance'),
    ('f', 'Faction');

CREATE TABLE Entity (
    id                  bigint PRIMARY KEY,
    name                text UNIQUE,
    entity_type         char REFERENCES(EntityType),

    guild               bigint NULL REFERENCES(Entity),
    aliance             bigint NULL REFERENCES(Entity),
    faction             bigint NULL REFERENCES(Entity)
);


