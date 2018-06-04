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

During retransmits of the same message the acks and CRC will change. We need to
add information about acks into the CTR-value so that we never reuse the CTR-value:

CTR = nonce + (sequenceNr:64, acknowledgeNr:6, popcount(acknowledgeMask):5, blockNr:12)

Maybe a special ACK message can be created that does not encrypt anything,
and may be smaller (only half the header).


```
struct Header[16] {
    uint2_t             type;               // Plain-text
    uint6_t             sequenceNr;         // Plain-text
    bool                fragment;           // Plain-text
    bool                message;            // Plain-text
    uint6_t             acknowledgeNr;      // Plain-text
    uint16_t            length;             // Plain-text
    uint32_t            acknowledgeMask;    // Plain-text
    uint32_t            reliabilityMask;    
    uint32_t            crc32c;
};

struct OpenPDU {
    uint8_t             publicKey[];        // Plain-text
    uint8_t             data[];
};

struct ClosePDU {
    uint8_t             data[];
};

struct DataPDU {
    uint8_t             data[];
};

struct ErrorPDU {
    uint32_t            errorCode;      // Plain-text
    uint8_t             errorMessage[]; // Plain-text
};

struct Packet[header.length] {
    Header              header;

    switch (header.type) {
    case 0: OpenPDU     pdu;
    case 1: DataPDU     pdu;
    case 2: ClosePDU    pdu;
    case 3: ErrorPDU    pdu;
    }
};

```

### Type

 * 0 - Open connecion
 * 1 - Established connection
 * 2 - Close connection
 * 3 - Error

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
The least significant 8 bits of the virtual 64 bit sequence number.
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
The least significant 8 bits of the sequence number of the last packet received.
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

The whole packet (except for the first byte of a packet and the public key) will be
encypted using AES in CTR mode. The counter is a 128-bit little-endian integer which
begins at the initial-value (IV) adding the blockNr and sequenceNr to it:

```
counter <= IV + (sequenceNr << 64 | blockNr);
```

### Checksum
A CRC-32C checksum of the complete non-encrypted packet including the header.
This encrypted checksum will count as proof that the correct AES key was used
and that the header was not modified by a middle box.

The CRC-32C is part of the header, during calculation of the CRC-32C the value in
the header should be set to zero.

### Data
The data that will be send to the application.

### Error Code

 *     0 -       No Error (Which is an error)
 *     1 -       Checksum incorrect - possible wrong AES-key used.
 *     2 -       Incorrect sequence number.
 *     3 -       Incorrect flags.
 *     4 -       Non empty sequence numbers during open.
 *       -   255 Reserved by RITP
 *   256 - 65535 Reserced by RITP Services
 * 65556 -       Application errors

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




