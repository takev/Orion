# Compressed Binary Serial Object Notation

## Format

```
stream                  = +(field | keep-alive);
field                   = integer | binary-float | decimal-float | boolean | none | string | byte-array | custom-value | list | dictionary | object | copy-operator;
boolean                 = true | false;
string                  = ascii-string | utf8-string;

integer                 = 0x00 byte:value
                        | 0x01 2*byte:value
                        | 0x02 3*byte:value
                        | 0x03 4*byte:value
                        | 0x04 5*byte:value
                        | 0x05 6*byte:value
                        | 0x06 7*byte:value
                        | 0x07 8*byte:value
                        | 0x08 2*byte:length <length>*byte:value
                        ;
                        
binary-float            = 0x09 binary16
                        | 0x0a binary32
                        | 0x0b binary64
                        | 0x0c binary128
                        ;

decimal-float           = 0x0d decimal32
                        | 0x0e decimal64
                        | 0x0f decimal128
                        ;

byte-array              = 0x10 byte:length <length>*byte:value
                        | 0x11 2*byte:length <length>*byte:value
                        | 0x12 3*byte:length <length>*byte:value
                        | 0x13 4*byte:length <length>*byte:value
                        ;

custom-value            = 0x14 field:name byte:length <length>*byte:value
                        | 0x15 field:name 2*byte:length <length>*byte:value
                        | 0x16 field:name 3*byte:length <length>*byte:value
                        | 0x17 field:name 4*byte:length <length>*byte:value
                        ;

keep-alive              = 0x18;
none                    = 0x19;
false                   = 0x1a;
true                    = 0x1b;
end                     = 0x1c;
list                    = 0x1d *field:value end;
dictionary              = 0x1e *(field:key field:value) end;
object                  = 0x1f field:name *(field:key field:value) end;

ascii-string            = 0x20-0x7e:character *0x01-0x7f:character 0x80-0xff:character;
utf8-string             = 0x7f *0x01-0xff:character 0x00;

copy-operator           = 0x80-0xff:distance;
```

## Initial Compression Dictionary

 | Distance | Type         |                Value |
 | -------- | ------------ | -------------------- |
 |        0 | String       |                empty |
 |        1 | List         |                empty |
 |        2 | Dictionary   |                empty |
 |        3 | Byte-array   |                empty |
 |        4 | Binary-float |                  0.0 |
 |        5 | Binary-float |                  1.0 |
 |        6 | Binary-float |                 -1.0 |
 |        7 | Binary-float |                  inf |
 |        8 | Binary-float |                 -inf |
 |        9 | Integer      |                    0 |
 |       10 | Integer      |                  127 |
 |       11 | Integer      |                 -128 |
 |       12 | Integer      |                  255 |
 |       13 | Integer      |                  256 |
 |       14 | Integer      |                32767 |
 |       15 | Integer      |               -32768 |
 |       16 | Integer      |                65535 |
 |       17 | Integer      |                65536 |
 |       18 | Integer      |           2147483647 |
 |       19 | Integer      |          -2147483648 |
 |       20 | Integer      |           4294967295 |
 |       21 | Integer      |           4294967296 |
 |       22 | Integer      |  9223372036854775807 |
 |       23 | Integer      | -9223372036854775808 |
 |       24 | Integer      | 18446744073709551615 |
 |       25 | Integer      | 18446744073709551616 |


## Fields

### Integers
Integers are encoded as signed two's compliment little endian. Integers must
be encoded in the smallest amount of space possible.

### Strings
When all characters code points in a string can be represented with 7 bits and
the first character is printable, then the string is stop-bit encoded.
The bit-7 is the stop-bit, denoting that it is the last character of a string.
The string should not end with a NUL character, unless the string is a single
character.

If a string contains characters that cannot be represented in 7 bits or
is a empty string. then the string is encoded as UTF-8 ending in a NUL character.

### Binary float
Binary floating point numbers should be encoded using the smallest amount of space
without losing precission.

### Decimal float
Decimal floating point numbers should be encoded using the smallest amount of space
without losing precission.

### Byte array
A byte array contains the length of the opaque data following it. The length is
encoded as a unsigned little endian integer. Always encode the length in the
smallest amount of bytes needed.

### Custom-value
A custom-value is like a byte-array, but includes the name of the type that is being
(de)serialized.

A custom-value contains the length of the data following it. The length is
encoded as a unsigned little endian integer. Always encode the length in the
smallest amount of bytes needed.

### None
A None value and type.

### Boolean
Can encode either True or False.

### List
A list contains a set of values, ending with the 'end' token.

### Dictionary
A dictionary contains a set of key value pairs.

### Object
A object is simular to a dictionary but includes the name of the type that is being
(de)serialized.

### Copy
The copy-operator is replaced by a single field from the compression-window at
a distance of 0 to 127. The result of the copy is in itself added to the
compression-window.

All fields that are encoded as 2 or more bytes are added to the compression-window.

The header (type length & name) of a byte-array, custom-field & object are added to
the compression-window before the compount byte-array, custom-field & object value.

The individual fields (keys & values) of a list, dictionary & object are added to the
compression-window after the compount list, dictionary & object value. 

### Keep-alive
This character can be send between fields to keep the network connection alive.

