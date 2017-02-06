## "BINJSON" format description

**note** - the format is **NOT COMPATIBLE** with any existing format defined elsewhere.

The format has been designed really straightforward for accommodate to the structure represented by the **imtjson** library. The main goal is the fast parsing and serializing.

There is no headers. The very first byte is parsed as an opcode. The parser always reads one item (which is defined as a single opcode or an opcode followed by some additional data). However some containers may contain many items in an unlimited structure, so "to read one item" may actually ends by a reading a large portion of the data.

### Table of opcodes

| opcode (hex) | additional bytes | name | description |
|---|---|---|---|
| 00 | n/a | padding | ignored by the parser |
| 01 | n/a | null | a value **null** |
| 02 | n/a | undefined | a value **undefined** |
| 03 | n/a | true |  a value **true** |
| 04 | n/a | false | a value **false** |
| 05 | 4 bytes | float | a float number (currently not serialized) |
| 06 | 8 bytes | double | a double number |
| 07-0E | n/a | reserved | reserved for future usage |
| 0F | n/a | diff | appears before object and tags that object as diff (arrays will be supported by a future version)|
| 1X | 0-8 bytes (size) + payload | binary | a binary string. The opcode is read as **posint**, and specifies the size of the string. The string's content immediatelly follows up to specified size |
| 2X | 0-8 bytes (value) | posint | unsigned integer |
| 3X | 0-8 bytes (value)| negint | negative integer (stored as unsigned integer) |
| 4X | 0-8 bytes (size) + payload | string | a  string. The opcode is read as **posint** and specifies the size of the string. The string's content immediatelly follows up to specified size |
| 5X | 0-8 bytes (size) | object | an object. The keys and the items immediatelly follow without any separator up to specified size. **The items must be ascii-ordered by the keys**  |
| 6X | 0-8 bytes (size) | array | an array. The items immediatelly follow without any separator up to specified size. The keys are allowed and optional, no ordering is required |
| 7X | 0-8 bytes (size) + payload + item | key | a key. It is stored as the string. The parser always expects an item after the key |
| 80 | n/a | keyRef0 | recently remebered key (first key in the FIFO). The parser always expects an item after the key |
| 81 | n/a | keyRef1 | second recently remebered key (second key in th FIFO). The parser always expects an item after the key |
| 82-FF | n/a | keyRef | remaining remembered keys: (opcode - 127) th key counted from the most recent to the oldest. The parser always expects an item after the key |

### decoding opcodes with arguments (the X in the opcode)

For the opcodes 10-7F, the lower nibble (lower four bits} contains the value or size of the value which immediately follows 

| opcode | bytes follow | meaning |
|---|---|---|
| X0-X9 | 0 | direct value 0-9 |
| XA | 1 | value is stored as uint8 following the opcode |
| XB | 2 | value is stored as uint16 following the opcode |
| XC | 4 | value is stored as uint32 following the opcode |
| XD | 8 | value is stored as uint64 following the opcode |
| XE | n/a | invalid / reserved |
| XF | n/a | invalid / reserved |

Examples
```
0x21 - value:1
0x35 - value: -5
0x2A 0x20 - value: 32
0x2B 0x00 0x80 - value: 32768 ('endian' ordering depend on a platform)
```

### decoding strings and keys

There is a size of the string encoded as unsigned integer number (with  a different higher nibble of the opcode)

```
<size><string>
```

Examples
```
0x45 0x68 0x65 0x6c 0x6c 0x6f - "hello"
0x4A 0x0B 0x68 0x65 0x6c 0x6c 0x6f 0x20 0x77 0x6f 0x72 0x6c 0x64 - "hello world"
```

### decoding containers

There is a size of the container encoded as unsigned integer number (with  a different higher nibble of the opcode)

```
<size><item><item>...
```


Examples
```
0x51 0x75 0x68 0x65 0x6c 0x6c 0x6f 0x45 0x77 0x6f 0x72 0x6c 0x64 - {"hello":"world"}
0x62 0x45 0x68 0x65 0x6c 0x6c 0x6f 0x45 0x77 0x6f 0x72 0x6c 0x64 - ["hello","world"]
```
### compressed keys

The serializer can use opcodes 0x80-0xFF to refer already transfered keys. It can refer up to the recent 128 keys. The keys out of the reach must be transfered again. A key received through these opcodes is not considered as recent. The dictionary of the keys isn't modified by any way.


Exaples
```
Assume that 6th key is "foo"
0x62 0x51 0x85 0x03 0x51 0x85 0x04 [{"foo":true},{"foo": false}]  

0x62 0x51 0x75 0x68 0x65 0x6c 0x6c 0x6f 0x03 0x51 0x80 0x04 [{"hello":true},{"hello": false}]
```


