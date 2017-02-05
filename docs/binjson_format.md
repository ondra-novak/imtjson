## "BINJSON" format description

**note** - the format is **NOT COMPATIBLE** with any existing format defined elsewhere.

No headers. The very first byte belongs to payload.

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
| 0F | n/a | diff appears before object and signals, that object is diff |
| 1X | 0-8 bytes (size) + payload | binary | a binary string. The opcode is read as **posint**, and specifies the size of the string. The string's content immediatelly follows up to specified size |
| 2X | 0-8 bytes (value) | posint | unsigned integer |
| 3X | 0-8 bytes (value)| negint | negative integer (stored as unsigned integer) |
| 4X | 0-8 bytes (size) + payload | string | a  string. The opcode is read as **posint** and specifies the size of the string. The string's content immediatelly follows up to specified size |
| 5X | 0-8 bytes (size) | object | a object. Immediatelly follows keys and items without a separator up to specified size. Note: items must be ordered by keys |
| 6X | 0-8 bytes (size) | array | a object. Immediatelly follows keys and items without a separator up to specified size  |
| 7X | 0-8 bytes (size) + payload | key | a key. Immediatelly after the key an item follows. The recent 128 keys are also remebered |
| 80 | n/a | keyRef0 | recently rembered key |
| 81 | n/a | keyRef1 | second recently rembered key |
| 82-FF | n/a | keyRef | other remebered keys |

### decoding opcodes with arguments (the X in the opcode)

For opcodes 10-7F, there lowest nibble can store the value or size of the value which immediatelly follows

| opcode | bytes follows |meaning |
|---|---|---|
| X0-X9 | 0 | direct value 0-9 |
| XA | 1 | value is stored as uint8 following the opcode |
| XB | 2 | value is stored as uint16 following the opcode |
| XC | 4 | value is stored as uint32 following the opcode |
| XD | 8 | value is stored as uint64 following the opcode |
| XE | n/a | invalid |
| XF | n/a | invalid |

Examples
```
0x21 - value:1
0x35 - value: -5
0x2A 0x20 - value: 32
0x2B 0x00 0x80 - value: 32768 ('endian' ordering depend on a platform)
```

### decoding strings and keys

There is size of the string encoded as unsigned integer which follows the above schema

Examples
```
0x45 0x68 0x65 0x6c 0x6c 0x6f - "hello"
0x4A 0x0B 0x68 0x65 0x6c 0x6c 0x6f 0x20 0x77 0x6f 0x72 0x6c 0x64 - "hello world"
```

### decoding containers

The is size of the containes encoded as unsigned integer which also follows the above schema

Examples
```
0x51 0x75 0x68 0x65 0x6c 0x6c 0x6f 0x45 0x77 0x6f 0x72 0x6c 0x64 - {"hello":"world"}
0x62 0x45 0x68 0x65 0x6c 0x6c 0x6f 0x45 0x77 0x6f 0x72 0x6c 0x64 - ["hello","world"]
```
### compressed keys

The serializer can use opcodes 0x80-0xFF to refer already defined keys. It can refer recent 128 keys. The keys out of the reach must be stored again.

Refering the key through the opcodes 0x80-0xFF doesn't affect currently stored keys. There is no reodering, redefining, etc. It just picks (opcode - 0x7F)th recent key and uses it.

Exaples
```
Assume that 6th key is "foo"
0x62 0x51 0x85 0x03 0x51 0x85 0x04 [{"foo":true},{"foo": false}]  

0x62 0x51 0x75 0x68 0x65 0x6c 0x6c 0x6f 0x03 0x51 0x80 0x04 [{"hello":true},{"hello": false}]
```


