# RIgel Transport Protocol

## RITP over UNIX domain socket.
Messages can be send on UNIX domain sockets without encapulation, encryption
are checksum.

```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |                                                               |
    /                                                               /
    /                           Payload                             /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## RITP over TCP/SSL.
When connecting over TCP the connection must be encrypted using SSL.
Messages need to be encapsulated with a length field and checksummed.

Here is the message format:

```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |K|                         Length                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                          Checksum                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                                                               |
    /                                                               /
    /                           Payload                             /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Key Exchange
For performance reasons RITP-over-TCP connection can be upgraded to
RITP-over-UDP. For this they need to notify the remote application
of the key it will use, the remote application will reply in kind.

Each application will have its own key with which it will use when
sending data to other hosts. An application may need to bind a
different UDP port than the TCP port, or may open multiple UDP ports.
The application must use a different key for each destinations.

This is the payload:
```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |1|                         Length=20                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                          Checksum                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |        SRC UDP Port           |         DST UDP Port          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 12 |                                                               |
    |                                                               |
 16 |                                                               |
    |                               Key                             |
 20 |                                                               |
    |                                                               |
 24 |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```    

## RITP over UDP.
```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |                                                               |
    |            SessionID          +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-|
  4 |                               | Type  |   SeqNr   |   AckNr   |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                            AckMask                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 12 |                            Checksum                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 16 |                                                               |
    /                                                               /
    /                            Payload                            /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

```

### SessionID
A 48-bit session ID, randomly generated at the start of the session.
A session ID should not be reused as it is used as part of the encryption
counter.

### Type

 0 - Start
 1 - Start-Ack
 2 - Finish
 3 - Finish-Ack
 4 - End-of-message
 5 - Unfinished-message
 6 - SessionID Error
 7 - Decrypt Error

#### Start

### SeqNr
The lower 6 bits of the 64 bit sequence number of the current packet.

### AckNr
The lower 6 bits of the 64 bit sequence number of the last packet received upto
32 packets after a non-received packet.

### AckMask
Each bit, starting from the LSB, shows which packets where received before
the packet denoted by the AckNr.

### Checksum
A CRC32-C result of the canonical header followed by the unencrypted data.
The payload is padded to a 32-bit boundary with zeros.

Following is the format of the data that is being checked:
```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |                           Session ID                          |
    +                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                               |            Length             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                                                               |
    +                           Sequence NR                         +
 12 |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 16 |                                                               |
    /                                                               /
    /                             Payload                           /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Encryption
All fields in the payload are encrypted using AES128 in CTR mode.
Due to the header being 128-bits, the payload will be aligned to
a 128-bit word boundary.

Following is how the CTR is build up:
```
       3                   2                   1
     1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |                           Session ID                          |
    +                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                               |            Offset             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                                                               |
    +                           Sequence NR                         +
 12 |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

The `Offset` field contains the number of 128-bit blocks inside the payload
of a packet.


