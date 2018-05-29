# RIgel Transport Protocol

Features:

 * Low latency / Low bandwidth
 * Reliable or Best efford
 * Fast retransmit.
 * Encrypted.
 * UDP or TCP carrier.

## Packet format

```
struct Header[16] {
    bool                open;
    bool                close;
    uint6_t             sequenceNr;
    uint56_t            reliabilityMask;
    bool                error;
    bool                fragment;
    uint6_t             acknowledgeNr;
    uint56_t            acknowledgeMask;
};

struct KeyExchangeHeader {
    uint128_t           nonce;
    uint8_t             publicKey[];
};

struct EncryptedData {
    uint32_t            checksum;
    uint8_t             data[];
};

struct ClientKeyExchangePacket {
    Header              header;
    KeyExchangeHeader   keyExchangeHeader;
    EncryptedData       encryptedData;
};

struct DataPacket {
    Header              header;
    EncryptedData       encryptedData;
};

struct ErrorPacket {
    Header              header;
    uint32_t            errorCode;
};

```
### Header
To frustrate deep packet inspection of the non-encrypted header; the first 16 bytes of
the packet (the header) is XOR-ed with the second 16 bytes of the packet (encryptedData
or keyExchangeHeader).

### OPEN
This flag marks the first packet send by the client to the server. In this
case the we use the ClientKeyExchangePacket format; otherwise use the DataPacket format.

During the Open the client will send its Diffie-Hellman public key, its AES-Nonce value,
the encrypted-checksum and optionally encrypted-data. This requires the client to already
know the Diffie-Hellman public key of the server, which it has received out-of-band.

### CLOSE
This flag marks the half-close packet from either the client or server. The close from
either side must be acknowledged. The Close flag may be set during Open. Data may be
send on the Close.

### ERROR
When this flag is set the packet is in the ErrorPacket format. It send when after receiving
a bad packet, or when the protocol is in a incorrect state.

The Close flag may be send on Error.

### DATA (virtual)
This flag is set when the length of the data is larger than zero.

### FRAGMENT
When this bit is set the data in this packet should be concatenated with the data
in the next packet. The application should only see a single large message.

### Sequence Number
The least significant 6 bits of the virtual 64 bit sequence number.
Sequence number zero represents the sequence number before opening the connection
and is used to acknowledge an OPEN packet without responding with data.

The sequence number is incremented before the sender sends a packet with  one or more
of the following flags: OPEN, CLOSE, DATA.

### Reliability Mask
This bit-mask references previously send packets, up to and including the current
packet. The reference to the current packet is the least significant bit of the
first byte of the mask.

 * 0 - This packet is will not be retransmitted.
 * 1 - This packet is will be retransmitted when not acknowledged.

Messages are always passed to the application in the correct order. But unreliable
messages may never arrive.

### Acknowledge Number
The sequence number of the last packet received. As long as references
to all the unreceived reliable-packets fit in the acknowledge-mask.

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
The Diffie-Hellman public key. A random number of the same size as the MODP group size
of the server. Must be a multiple of 128-bits in size for proper alignment of the 
encrypted data.

To protect against man-in-the-midde attack; the Diffie-Hellman server-public-key MUST
be distributed out-of-band and is not transmitted in-band. This also saves in data to
be transmitted during the handshake.

### Nonce
A 128-bit random number used as part of the AES-CTR counter.

### Encrypted Data
The data on the connection is encrypted using AES in CTR-mode. The start of the
data is aligned to 128-bit so that we can use x86 SSE/AES/CRC instructions
directly on a receive/send buffer. The AES-Key is negotiated using Diffie-Hellman during OPEN.
The AES-Counter is build up as follows:

```
counter <= nonce XOR (serverSide << 127 | sequenceNr << 32 | blockNr);
```

### Checksum
A CRC-32C checksum of the complete non-encrypted packet including the header and optional
KeyExchangeHeader. This encrypted checksum will count as proof that the correct AES key was used
and that the header was not modified by a middle box.

### Data
Encrypted application data. If data is zero length, then no message is send to the application.

### Error Code

 *     0 -       No Error (Which is an error)
 *     1 -       Checksum incorrect - possible wrong AES-key used.
 *     2 -       Incorrect sequence number.
 *     3 -       Incorrect flags.
 *     4 -       Non empty sequence numbers during open.
 *       -   255 Reserved by RITP
 *   256 - 65535 Reserced by RITP Services
 * 65556 -       Application errors

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




