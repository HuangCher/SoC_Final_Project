# Bloom Accelerator Register Interface

## Register Map
| ADDR | PURPOSE |
|------|---------|
| 0x00 | KEY_IN  |
| 0x04 | CONTROL |
| 0x08 | RESULT  |
| 0x0C | STATUS  |

## CONTROL bits
|   BIT     |   PURPOSE |
|-----------|-----------|
|   bit 0   | START     |
|   bit 1   | INSERT    |
|   bit 2   | QUERY     |
|   bit 3   | CLEAR     |

## Command Values (WRITE TO CONTROL)
- INSERT = 0x3   (START | INSERT)
- QUERY  = 0x5   (START | QUERY)
- CLEAR  = 0x9   (START | CLEAR)

## STATUS bits
- bit 0 - DONE
- bit 1 - BUSY

## RESULT
bit:
- 0 = query result
- 1 = maybe present
- 0 = definitely not present

## Software Usage

INSERT:
1. wait until BUSY = 0
2. write KEY_IN
3. write CONTROL = 0x3
4. wait until DONE = 1

QUERY:
1. wait until BUSY = 0
2. write KEY_IN
3. write CONTROL = 0x5
4. wait until DONE = 1
5. read RESULT

CLEAR:
1. wait until BUSY = 0
2. write CONTROL = 0x9
4. wait until DONE = 1

## Command Behavior Rules

### Valid commands
A valid command is a write to CONTROL with START set and exactly one operation bit set.

Valid values:
- 0x3 = INSERT
- 0x5 = QUERY
- 0x9 = CLEAR

Any other CONTROL value is treated as invalid and ignored.

### Command start behavior
When a valid command is accepted:
- the accelerator begins processing that command
- BUSY goes to 1
- DONE goes to 0

Software must not issue a new command while BUSY = 1.

### Command completion behavior
When the command finishes:
- BUSY goes to 0
- DONE goes to 1

### RESULT behavior
- RESULT bit 0 is only meaningful after a QUERY completes
- RESULT = 1 means maybe present
- RESULT = 0 means definitely not present
- INSERT and CLEAR do not need to change RESULT

### KEY_IN behavior
- KEY_IN is used by INSERT and QUERY
- CLEAR ignores KEY_IN

### Reset behavior
After reset:
- BUSY = 0
- DONE = 0
- RESULT = 0
- Bloom filter storage is all zeros

### Invalid software behavior
The following are invalid:
- issuing a command while BUSY = 1
- writing START with multiple operation bits set
- writing START with no operation bit set