
= Compressed Binary Object Notation
The following features where the primary motifations:
 * Simple format
 * Fast encoding and decoding
 * Compression



== Literal
A literal token has the following format: 0ILLLTTT

=== Integer
Integers are ZigZag encoded.

Opcode:
 * 000VVVV
 * 001LLLL N=L*(byte) N*(byte)

| Length | Description  |
| ------:|:------------ |
|      0 | N = 0  L = 0 |
|      1 | N = 1  L = 0 |
|      2 | N = 2  L = 0 |
|      3 | N = 3  L = 0 |
|      4 | N = 4  L = 0 |
|      5 | N = 5  L = 0 |
|      6 | N = 6  L = 0 |
|      7 | N = 7  L = 0 |
|      8 | N = 8  L = 0 |
|      9 | N = 9  L = 0 |
|     10 | N = 10 L = 0 |
|     11 | N = 11 L = 0 |
|     12 | N = 12 L = 0 |
|     13 |        L = 1 |
|     14 |        L = 2 |
|     15 |        L = 4 |

=== Byte Array

Opcode: 0100LLLL N=L*(byte) N*(byte)

| Length | Description  |
| ------:|:------------ |
|      0 | N = 0  L = 0 |
|      1 | N = 1  L = 0 |
|      2 | N = 2  L = 0 |
|      3 | N = 3  L = 0 |
|      4 | N = 4  L = 0 |
|      5 | N = 5  L = 0 |
|      6 | N = 6  L = 0 |
|      7 | N = 7  L = 0 |
|      8 | N = 8  L = 0 |
|      9 | N = 9  L = 0 |
|     10 | N = 10 L = 0 |
|     11 | N = 11 L = 0 |
|     12 | N = 12 L = 0 |
|     13 |        L = 1 |
|     14 |        L = 2 |
|     15 |        L = 4 |

=== Simple Types

| Opcode             | Description
|:------------------ | ----
| 01010000           | False
| 01010001           | True
| 01010010           | None
| 01010011 *(byte) 0 | UTF-8 String
| 01010100 *(byte) 0 | Interned UTF-8 String
| 01010101 4*(byte)  | float
| 01010110 8*(byte)  | double
| 01010111 16*(byte) | UUID
| 01011000 8*(byte)  | # nanoseconds since 2010.
| 01011001           | 
| 01011010           | 
| 01011011           | 
| 01011100           | 
| 01011101           | 
| 01011110           | 
| 01011111           | 

=== List

Opcode: 0110LLLL N=L*(byte) N*(value:literal)

| Length | Description  |
| ------:|:------------ |
|      0 | N = 0  L = 0 |
|      1 | N = 1  L = 0 |
|      2 | N = 2  L = 0 |
|      3 | N = 3  L = 0 |
|      4 | N = 4  L = 0 |
|      5 | N = 5  L = 0 |
|      6 | N = 6  L = 0 |
|      7 | N = 7  L = 0 |
|      8 | N = 8  L = 0 |
|      9 | N = 9  L = 0 |
|     10 | N = 10 L = 0 |
|     11 | N = 11 L = 0 |
|     12 | N = 12 L = 0 |
|     13 |        L = 1 |
|     14 |        L = 2 |
|     15 |        L = 4 |

=== Dictionary

Opcode: 0111LLLL N=L*(byte) N*(key:literal value:literal)

| Length | Description  |
| ------:|:------------ |
|      0 | N = 0  L = 0 |
|      1 | N = 1  L = 0 |
|      2 | N = 2  L = 0 |
|      3 | N = 3  L = 0 |
|      4 | N = 4  L = 0 |
|      5 | N = 5  L = 0 |
|      6 | N = 6  L = 0 |
|      7 | N = 7  L = 0 |
|      8 | N = 8  L = 0 |
|      9 | N = 9  L = 0 |
|     10 | N = 10 L = 0 |
|     11 | N = 11 L = 0 |
|     12 | N = 12 L = 0 |
|     13 |        L = 1 |
|     14 |        L = 2 |
|     15 |        L = 4 |

