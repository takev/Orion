# Rigel Serial Object Notation

## Format

```
field           = simplex | complex;

simplex         = integer | boolean | none | string;
complex         = float | list | dictionary | byte-array;

boolean         = true | false;
string          = ascii-string | utf8-string;
float           = binary-float | decimal-float

mark            = 0x00;
none            = 0x10;
true            = 0x11;
false           = 0x12;
list            = 0x13 *field:value mark;
dictionary      = 0x14 *field:key mark *field:value;
decimal-float   = 0x15 (integer:mantissa integer:exponent | integer:0 | true:-inf | false:inf);
binary-float    = 0x16 (integer:mantissa integer:exponent | integer:0 | true:-inf | false:inf);
byte-array      = 0x17 +(0x00-0xff:length <length>*byte);
utf8-string     = 0x18 *0x01-0xff:value mark;
reserved        = 0x19;
reserved        = 0x1a;
ascii-chr       = 0x01-0x0f | 0x1b-0x7f;
last-ascii-char = 0x80-0x8f | 0x9b-0xff;
ascii-string    = +ascii-char:value last-ascii-char:value
integer         = 0b1SVVVVVV *0bSVVVVVVV;
```

## Fields

### Integers
Integers are encoded as signed two's compliment variable length little endian
stop-bit encoded.

The first byte has 6 bits of value and bit-6 is the stop-bit. Following
bytes has 7 bits of value and bit-7 is the stop-bit. The last byte has
the stop-bit set.

Integers must be encoded with the least amount of bits needed. Sign-bits always
need to be encoded, so it may be required to have a full byte needed for
just the sign-bits.

### Strings
When all characters code points in a string are in 0x01-0x0f and 0x1b-0x7f and
is at least one character long, then the string must be encoded as an
ascii-string. And ascii-string is encoded as 7 bits with a stop-bit in
the most-significant bit of each byte. When the stop-bit is set this
character is the last of the string. An ascii-string must not end with a
NUL character, unless it is only a single character.

Any other string must be encoded as a utf8-string. An utf8-string is
ends with a mark-token. The string must contain only valid utf-8 values.

### Byte array
A byte array is encoded in chucks of up to 255 bytes, using a byte for the size
of the next chuck. A chunk of less than 255 bytes terminates the byte array.

If a name is included the byte array is used to encode a specific custom-type.
A name must be a string of at least 1 character or none.

### Binary float
Binary floating point numbers represent: mantissa * 2^exponent.

The exponent is scaled such that the mantissa does not have trailing zero bits.
The floating point number should be encoded in the least amount of bytes without
losing precision from the original representation.

Different from IEEE-754 floating point the mantissa explicitly encodes
the most-significant bit and the exponent is encoded without bias. Due to
not explicitly encoding the most-significant bit there are no denormals.

NaN is encoded as none plus an integer for the code.
positive infinite is encoded as true, and negative infinite is encoded as false.

### Decimal float
Binary floating point numbers represent: mantissa * 10^exponent.

The exponent is scaled such that the mantissa does not have trailing zero decimal
digits. The floating point number should be encoded in the least amount of bytes
without losing precision from the original representation.

### None
A None value and type.

### Boolean
Can encode either True or False.

### List
A list contains a set of values, ending with the mark token.

### Dictionary
A dictionary contains a set of key value pairs. First keys are send,
followed by the mark token. Then all values are send, followed by the second
mark token.

Keys must be unique within an dictionary.

The keys are send first, so it is easier to compress using a lzw-like algorithm.
For this and for canonicallity reasons the keys must be sorted.

If a name is included then the dictionary is used to (de)serialize a class-instance.
A name must be a string of at least 1 character or none.

### Mark
The mark token is technically not a field. The mark token is not allowed to start
a field, or it would not be possible to differentiate a field from the end of a list
or dictionary.

Since the only value that is not allowed in a UTF-8 string is 0x00 it is a useful
value for the mark-code.

## Sorting
Keys in an dictionary must be sorted for canonical reasons. The following sort ordering
must be used.

* None
* False
* True
* Ascending integers
* Ascending binary float (-infinite, ascending value, +infinite, NaN code).
* Ascending decimal float (-infinite, ascending value, +infinite, NaN code).
* Strings, sorted left to right for each byte value in its UTF-8 encoded form.
* Byte-array, sorted by name, then left to right for each byte value.
* list, sorted left to right
* dictionary, sorted by name, then by keys left to right, then by values left to right.

