# Compressed Binary Serial Object Notation

## Format

```
field           = simplex | complex;

simplex         = integer | boolean | none | string;
complex         = float | list | object | byte-array;

boolean         = true | false;
string          = ascii-string | utf8-string;
float           = binary-float | decimal-float

mark            = 0x00;                                                                                    # ASCII-NUL
none            = 0x10;                                                                                    # ASCII-DC1
true            = 0x11;                                                                                    # ASCII-DC2
false           = 0x12;                                                                                    # ASCII-DC3
list            = 0x13 *field:value mark;                                                                  # ASCII-DC4
object          = 0x14 field:name *field:key mark *field:value mark;                                       # ASCII-NAK
decimal-float   = 0x15 (integer:exponent integer:mantissa | none:nan integer:code | true:inf | false:-inf) # ASCII-SYN
binary-float    = 0x16 (integer:exponent integer:mantissa | none:nan integer:code | true:inf | false:-inf) # ASCII-ETB
byte-array      = 0x17 field:name +(0x00-0xff:length <length>*byte);                                       # ASCII-CAN
utf8-string     = 0x18 *0x01-0xff:value mark;                                                              # ASCII-EM
reserved        = 0x19;                                                                                    # ASCII-DLE
reserved        = 0x1a;                                                                                    # ASCII-SUB
ascii-chr       = 0x01-0x0f | 0x1b-0x7f;
last-ascii-char = 0x81-0x8f | 0x9b-0xff;
ascii-string    = +ascii-char:value last-ascii-char:value
integer         = 0b1CVVVVVV *0bCVVVVVVV;
```

## Fields

### Integers
Integers are encoded as signed two's compliment variable length little endian
continue bit encoded.

The first byte has 6 bits of value and bit-6 is the continue bit. Following
bytes has 7 bits of value and bit-7 is the continue bit.

Integers must be encoded with the least amount of bits needed. Sign-bits always
need to be encoded, so it may be required to have a full byte needed for
just the sign-bits.

### Strings
When all characters code points in a string are in 0x00-0x0f and 0x1b-0x7f and
is at least two characters long, then the string must be encoded as an
ascii-string. And ascii-string is encoded as 7 bits with a stop-bit in
the most-significant bit of each byte. When the stop-bit is set this
character is the last of the string. An ascii-string must not end with a
NUL character.

Any other string must be encoded as a utf8-string. An utf8-string is
ends with a mark-token.

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

### Object
A dictionary contains a set of key value pairs. First keys are send,
followed by the mark token. Then all values are send, followed by the second
mark token.

Keys must be unique within an object.

The keys are send first, so it is easier to compress using a lzw-like algorithm.
For this and for canonicallity reasons the keys must be sorted.

If a name is included then the object is used to (de)serialize a class-instance.
A name must be a string of at least 1 character or none.

### Mark
The mark token is technically not a field. The mark token is not allowed to start
a field, or it would not be possible to differentiate a field from the end of a list
or dictionary.

Since the only value that is not allowed in a UTF-8 string is 0x00 it is a useful
value for the mark-code.

## Sorting
Keys in an object must be sorted for canonical reasons. The following sort ordering
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
* object, sorted by name, then by keys left to right, then by values left to right.

