# RIgel Transport Protocol

Features:

 * Low latency / Low bandwidth
 * Reliable or Best efford
 * Fast retransmit.
 * Encrypted.
 * UDP or TCP carrier.

## Packet format
The complete packet including the header is encryped using AES128-CTR.

Exceptions to encryption:

 * The first 64 bits of the packet is in plain-text.
 * The publicKey in the OPEN packet is in plain-text.

The reason for the first 64 bits being in plain-text is:

 * Type is needed because during open we don't know the key yet.
 * SequenceNr is needed for the counter value of the encryption.
 * AcknowledgeNr is needed for the counter value of the encryption.
 * AkcnowledgeMask is needed for the counter value of the encryption.
 * Length is used inside the Linux kernel to split TCP messages into a message stream.

### OPEN, OPEN-FRAG, OPEN-CLOSE
The first 12 bytes are send in plain-text. This means an error message can include
de session-cookie in response.

```
    :    Byte 0     :    Byte 1     :    Byte 2     :    Byte 3     :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 | TYP |         Length          | VER |   Public Key Length     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                          Reserved = 0                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                         Session Cookie                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 12 |                            CRC-32C                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 16 |                                                               |
    /                                                               /
                                Public Key
    /                                                               /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    /                                                               /
                                   Data
    /                                                               /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### DATA, DATA-FRAG, DATA-CLOSE
The first 8 bytes are send in plain-text; the SeqNr, AckNr, AcknowledgeMask
are needed to create the CTR value for the AES-CTR-mode encryption/decryption.

This means that if the AckNr or AcknowledgeMask changes when the packet needs
to be retransmitted, then the packet needs the be re-encrypted with the new
CTR value. This is required so we don't reuse the CTR value and potentially
allow spoofing of the CRC-32C value.

```
    :    Byte 0     :    Byte 1     :    Byte 2     :    Byte 3     :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 | TYP |         Length          |     AckNr     |     SeqNr     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                        Acknowledge Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                        Reliability Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 12 |                            CRC-32C                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 16 |                                                               |
    /                                                               /
                                  Data
    /                                                               /
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### CONTROL
The first 8 bytes are send in plain-text. The sequence number used in the CTR
is set to 0xff'ffff'ffff'ffff.

The encrypted session cookie is used for authenticating this packet.
An CONTROL packet without a valid session cookie must be ignored.
There is only one exception, when the CONNTROL packet is in reply to
an OPEN packet, where the key-exchange may have failed.

The CONTROL packet is used for:

 * Acknowleding packets when there is no DATA packet ready to be (re-)send.
 * Keep-alive packets to keep firewalls from dropping the connection.
 * When an error needs to be reported.

In case of error Both peers will need to reply to non-error packets with an
error packet using the current Session Cookie for up to 60 seconds.

```
    :    Byte 0     :    Byte 1     :    Byte 2     :    Byte 3     :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |TYP=7|        Length = 12      |     AckNr     |  ControlCode  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  4 |                        Acknowledge Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |                         Session Cookie                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

This is the CONTROL message format for an error in reply to an OPEN packet
where the key-exchange has failed:
```
    :    Byte 0     :    Byte 1     :    Byte 2     :    Byte 3     :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |TYP=7|        Length = 4       |     AckNr     |  ControlCode  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## Fields

### VER - Version
Current version of the protocol is 0.

### TYPE
There are several different packet in this protocol:

| TYPE | Name       | Description                         |
| ----:| ---------- | ----------------------------------- |
|    0 | OPEN       | Open Connection                     |
|    1 | OPEN-FRAG  | Open Connection, data is fragmented |
|    2 | OPEN-CLOSE | Open Connection, then close         |
|    3 | DATA       | Transfer                            |
|    4 | DATA-FRAG  | Transfer, data is fragmented        |
|    5 | DATA-CLOSE | Transfer, then close                |
|    6 |            |                                     |
|    7 | CONTROL    | Acknowledge / Keep-alive / Error    |

#### Open connection
This is the first packet send by the client to the server.

During the Open the client will send its Diffie-Hellman public key.
This requires the client to already know the Diffie-Hellman public key of the server,
which it has received out-of-band.

Data may be send on the Open. The encrypted CRC-32C in the value in the header will
prove that the client has completed the cryptographic hand-shake.

#### Close connection
This type half-closes the connection, meaning that the sender will not send any more
date . The close from either side must be acknowledged.

Data may be send on the Close.

#### Established connection

#### Error
When this flag is set the packet is in the ErrorPacket format. It is send after receiving
a bad packet, or when the protocol is in a incorrect state.

None of the packet is encrypted on error, since error could be cuased by having no or 
incorrect encryption keys.

### MESSAGE
This flag is send when a message needs to be delivered to the application. It is
possible for the message to be zero bytes in length.

### FRAGMENT
When this bit is set the data in this packet should be concatenated with the data
in the next packet. The application should only see a single large message.

The maximum data size of fragmented message is 65536 bytes.

### Length
This is the length of the packet including the header. This is needed when encapsulating
RITP inside a streaming protocol such as TCP.

### Sequence Number
The least significant 6 bits of the virtual 56 bit sequence number.
Sequence number zero represents the sequence number before opening the connection
and is used when acknowledging an OPEN packet without responding with data.

The sequence number is incremented before the sender sends a OPEN- or CLOSE-packet, or
when the MESSAGE flag is set.

### Reliability Mask
This bit-mask references previously send packets, up to and including the current
packet. The reference to the current packet is the least significant bit of the
first byte of the mask.

 * 0 - This packet is will not be retransmitted.
 * 1 - This packet is will be retransmitted when not acknowledged.

Messages are always passed to the application in the correct order. But unreliable
messages may never arrive.

### Acknowledge Number
The least significant 6 bits of the sequence number of the last packet received.
As long as references to all the unreceived reliable-packets fit in the acknowledge-mask.

At the start this mask is zero, as if all messages before the OPEN where send unreliably.

### Acknowledge Mask
This bit-mask references previously received packets, up to and including the last
packet. The reference to the current packet is the least significant bit of the
first byte of the mask.

 * 0 - This packet is missing.
 * 1 - This packet has been received.

The sending application will receive ack/nack confirmation for any (previous) 
unreliable messages send as soon as a packet is acknoweledged. This is possible
because all messages are strictly ordered.

At the start this mask is zero, as if all messages before the OPEN have not been received.

### Public Key
The Diffie-Hellman public key in little endian format. A random number of the same size
as the MODP group size of the server.

To protect against man-in-the-midde attack; the Diffie-Hellman server-public-key MUST
be distributed out-of-band and is not transmitted in-band. This also saves in data to
be transmitted during the handshake.

The result of the key-exchange is stored in little-endian and hashed using SHA-512.
From the lsb to msb, the 512-bit value divided into four 128-bit values:

 * client-side-AES-key
 * server-side-AES-key
 * client-side-IV
 * server-side-IV

### AES Encryption of the packet
The whole packet (except for the first 64 bit) will be encypted using AES in CTR mode.


```

                     +-----------+        +---+
                     |    CTR    |        | 1 |
                     +-----------+        +---+
                           |                |
                           v                v
    +-----------+        +---+            +---+
    |    IV     |------->| + |------------| + |
    +-----------+        +---+            +---+
                           |                |
                           v                v
                     +-----------+    +-----------+
                     |    AES    |    |    AES    |
                     +-----------+    +-----------+
                           |                |
                           v                v
                 +----+  +---+    +----+  +---+
                 | M0 |->| ^ |    | M1 |->| ^ |
                 +----+  +---+    +----+  +---+
                           |                |
                           v                v
                        +----+           +----+
                        | C0 |           | C1 |
                        +----+           +----+
```


CTR for encrypting:
```
    :    Byte 0     :    Byte 1     :    Byte 2     :    Byte 3     :
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  0 |  Block Number   |POP(AckMsk)|0|                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
  4 |                         Acknowledge Nr                        |
    +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  8 |               |                                               |
    +-+-+-+-+-+-+-+-+        Full Sequence Nr                       +
 12 |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

This CTR is treated as a 128 bit unsigned integer with roll-over.
The IV that is created during the key-exchange process is simply
added to this CTR value before it is encrypted with AES.

### Checksum
A CRC-32C checksum of the complete non-encrypted packet including the header.
This encrypted checksum will count as proof that the correct AES key was used
and that the header was not modified by a middle box.

The CRC-32C is part of the header, during calculation of the CRC-32C the value in
the header should be set to zero.

### Data
The data that will be send to the application.

### Control Code

| Range    | Description
| --------:| ------------------------------------------------ |
|  0 -  15 | No error, first 8 bytes are plain-text           |
| 15 -  63 | OPEN error, packet is plain-text                 |
| 64 - 255 | Other error, first 8 bytes are plain-text        |

| Code | Description                                       |
| ----:| ------------------------------------------------- |
|    0 | No Error. Acknowledge or Keepalive.               |
|    1 | OPEN Error, could not decrypt.                    |
|    2 | Checksum incorrect - possible wrong AES-key used. |
|    3 | Incorrect sequence number.                        |
|    4 | Incorrect flags.                                  |
|    5 | Non empty sequence numbers during open.           |

### Error Message
Error message in english to be presented to a user.

## Dynamics
In the example we only show masks with 8 bits.

### Acknowledge without data
It is always possible to acknowledge without data by
using a sequence number that was already acknowledged by the peer.
This is why the first OPEN packet starts at sequence number 1.

### Example: Shortest transaction.

 1. C: SEQ=(1)00000001 ACK=(0)00000000 OPEN,DATA,CLOSE
 2. S: SEQ=(1)00000001 ACK=(1)00000001 DATA,CLOSE
 3. C: SEQ=(1)00000001 ACK=(1)00000001

### Example Open without data.

 1. C: SEQ=(1)00000001 ACK=(0)00000000 OPEN
 2. S: SEQ=(0)00000000 ACK=(1)00000001 
 3. C: SEQ=(2)00000011 ACK=(0)00000000 DATA
 4. S: SEQ=(0)00000000 ACK=(2)00000011 
 5. C: SEQ=(3)00000111 ACK=(0)00000000 CLOSE
 6. S: SEQ=(0)00000001 ACK=(3)00000111 CLOSE
 7. C: SEQ=(3)00000111 ACK=(1)00000001 


## Security, Encryption, Proof-of-work.
To protect against reflection and amplification attacks; the client MUST send a valid
encryptedChecksum, which is a proof-of-work. If the encryptedChecksum is
incorrect or if the client-public-key has been reused the server MUST not respond.

To protect against distributed-denial-of-service; we can use a group of authentication servers
and a load-balancer to forward already established connections to application servers. This
requires an higher level protocol to pass the AES-Key and Nonce from the
authentication servers to the application servers through the load balancer. This will
be discussed in a seperate document.




