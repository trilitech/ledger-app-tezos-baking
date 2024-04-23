# APDU

An APDU is sent by a client to the ledger hardware. This message tells
the ledger some operation to run.  The basic format of the APDU
request follows.

| Field   | Length   | Description                                                            |
|---------|----------|------------------------------------------------------------------------|
| *CLA*   | `1 byte` | Instruction class (always 0x80)                                        |
| *INS*   | `1 byte` | Instruction code (0x00-0x0f)                                           |
| *P1*    | `1 byte` | Index of the message (0x80 lor index = last index)                     |
| *P2*    | `1 byte` | Derivation type (0=ED25519, 1=SECP256K1, 2=SECP256R1, 3=BIP32_ED25519) |
| *LC*    | `1 byte` | Length of *CDATA*                                                      |
| *CDATA* | `<LC>`   | Payload containing instruction arguments                               |

Each APDU has a header of 5 bytes followed by some data. The format of
the data will depend on which instruction is being used.

## Example

Here is an example of an APDU message:

> 0x8001000011048000002c800006c18000000080000000

This parses as:

| Field   | Value                                  |
|---------|----------------------------------------|
| *CLA*   | `0x80`                                 |
| *INS*   | `0x01`                                 |
| *P1*    | `0x00`                                 |
| *P2*    | `0x00`                                 |
| *LC*    | `0x11` (17)                            |
| *CDATA* | `0x048000002c800006c18000000080000000` |

0x01 is the first instruction and is used for "authorize baking". This
APDU tells the ledger to save the choice of curve and derivation path
to memory so that future operations know which of these to use when
baking.

## Exceptions

| Exception                       | Code   | Short description                               |
|---------------------------------|--------|-------------------------------------------------|
| `EXC_WRONG_PARAM`               | 0x6B00 | Wrong parameter(s) *P1*-*P2*                    |
| `EXC_WRONG_LENGTH`              | 0x6C00 | Incorrect length.                               |
| `EXC_INVALID_INS`               | 0x6D00 | Instruction code not supported or invalid.      |
| `EXC_WRONG_LENGTH_FOR_INS`      | 0x917E | Length of command string invalid.               |
| `EXC_REJECT`                    | 0x6985 | Conditions of use not satisfied.                |
| `EXC_PARSE_ERROR`               | 0x9405 | Problems in the data field.                     |
| `EXC_REFERENCED_DATA_NOT_FOUND` | 0x6A88 | Referenced data not found.                      |
| `EXC_WRONG_VALUES`              | 0x6A80 | The parameters in the data field are incorrect. |
| `EXC_SECURITY`                  | 0x6982 | Security condition not satisfied.               |
| `EXC_CLASS`                     | 0x6E00 | Class not supported.                            |
| `EXC_MEMORY_ERROR`              | 0x9200 | Memory error.                                   |

## Instructions

| Instruction                                                      | Code | Short description                           |
|------------------------------------------------------------------|------|---------------------------------------------|
| [`VERSION`](apdu.md#version)                                     | 0x00 | Get version information for the ledger      |
| [`AUTHORIZE_BAKING`](apdu.md#authorize_baking)                   | 0x01 | Authorize baking                            |
| [`GET_PUBLIC_KEY`](apdu.md#get_public_key)                       | 0x02 | Get the ledger’s internal public key        |
| [`PROMPT_PUBLIC_KEY`](apdu.md#prompt_public_key)                 | 0x03 | Prompt for the ledger’s internal public key |
| [`SIGN`](apdu.md#sign)                                           | 0x04 | Sign a message with the ledger’s key        |
| [`RESET`](apdu.md#reset)                                         | 0x06 | Reset high water mark block level           |
| [`QUERY_AUTH_KEY`](apdu.md#query_auth_key)                       | 0x07 | Get auth key                                |
| [`QUERY_MAIN_HWM`](apdu.md#query_main_hwm)                       | 0x08 | Get current high water mark                 |
| [`GIT`](apdu.md#git)                                             | 0x09 | Get the commit hash                         |
| [`SETUP`](apdu.md#setup)                                         | 0x0a | Setup a baking address                      |
| [`QUERY_ALL_HWM`](apdu.md#query_all_hwm)                         | 0x0b | Get all high water mark information         |
| [`DEAUTHORIZE`](apdu.md#deauthorize)                             | 0x0c | Deauthorize baking                          |
| [`QUERY_AUTH_KEY_WITH_CURVE`](apdu.md#query_auth_key_with_curve) | 0x0d | Get auth key and curve                      |
| [`HMAC`](apdu.md#HMAC)                                           | 0x0e | Get the HMAC of a message                   |
| [`SIGN_WITH_HASH`](apdu.md#sign_with_hash)                       | 0x0f | Sign a message with the ledger’s key        |

### `VERSION`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x00` | `__` | `__` |

Get version information of the application.

#### Input data

No input data.

#### Output data

| Length | Description              |
|--------|--------------------------|
| `1`    | Should be 1 for `baking` |
| `1`    | The major version        |
| `1`    | The minor version        |
| `1`    | The patch version        |

### `AUTHORIZE_BAKING`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x01` | `0x00` | `P2` |

Requests authorization to bake with the key associated with the given
`path` and `P2`.

If no `path` is provided, the request is performed using the key
already authorized.

If the request is accepted, the key is defined as the [`authorized-key`](NVRAM.md#authorized-key)
and the public key is returned.

Refusing the request do not erase the existing [`authorized-key`](NVRAM.md#authorized-key)

#### Input data

| Length       | Description               |
|--------------|---------------------------|
| `<variable>` | The `path` (can be empty) |

#### Output data

| Length     | Description             |
|------------|-------------------------|
| `1`        | The public key `length` |
| `<length>` | The public key          |

### `GET_PUBLIC_KEY`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x02` | `0x00` | `P2` |

Get the public key according to the `path` and `P2`.

This instruction is not allowed for permissionless legacy comm in browser.

#### Input data

| Length       | Description |
|--------------|-------------|
| `<variable>` | The `path`  |

#### Output data

| Length     | Description             |
|------------|-------------------------|
| `1`        | The public key `length` |
| `<length>` | The public key          |

### `PROMPT_PUBLIC_KEY`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x02` | `0x00` | `P2` |

Requests to get the public key according to the `path` and `P2`.

#### Input data

| Length       | Description |
|--------------|-------------|
| `<variable>` | The `path`  |

#### Output data

| Length     | Description             |
|------------|-------------------------|
| `1`        | The public key `length` |
| `<length>` | The public key          |

### `SIGN`

#### First apdu

| *CLA*  | *INS*  | *P1*             | *P2* |
|--------|--------|------------------|------|
| `0x80` | `0x04` | `0x00` or `0x80` | `P2` |

Set the signing key to the key associated with the given `path` and
`P2`.

This step is not required, as long as the [`authorized-key`](NVRAM.md#authorized-key) has been
defined. In this case the signature will be performed by this
[`authorized-key`](NVRAM.md#authorized-key).

##### Input data

| Length       | Description |
|--------------|-------------|
| `<variable>` | The `path`  |

##### Output data

No output data.

#### Other apdus

| *CLA*  | *INS*  | *P1*             | *P2* |
|--------|--------|------------------|------|
| `0x80` | `0x04` | `0x01` or `0x81` | `__` |

Request to sign the `message`.

Use `P1 = 0x81` to indicate that the message has been fully sent.

Once the message has been fully sent and the request has been
accepted, the signature of the message is returned.

Messages sent in more than one packet will be refused.

If the `message` is a valid `baking message` (`Block` or `Consensus
operation`), no confirmation screens will be displayed and the
signature will be automatic.

See [the messages in the specification](signing.md#messages) and the [API](https://tezos.gitlab.io/shell/p2p_api.html).

##### Input data

| Length       | Description           |
|--------------|-----------------------|
| `<variable>` | The `message` to sign |

##### Output data

| Length       | Description   |
|--------------|---------------|
| `<variable>` | The signature |

### `RESET`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x06` | `__` | `__` |

Requests a reset of the minimum level authorised in the main and the
test chains to `level`.

Once accepted the two [`HWM`](NVRAM.md#hwm) will be set their level to `level` and
their round will be set to `0`.

#### Input data

| Length | Description |
|--------|-------------|
| `4`    | The `level` |

#### Output data

No output data.

### `QUERY_AUTH_KEY`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x07` | `__` | `__` |

Get the path of the [`authorized-key`](NVRAM.md#authorized-key).

#### Input data

No input data.

#### Output data

| Length       | Description |
|--------------|-------------|
| `<variable>` | The `path`  |

### `QUERY_MAIN_HWM`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x08` | `__` | `__` |

Get the [`HWM`](NVRAM.md#hwm) of the main chain.

#### Input data

No input data.

#### Output data

| Length | Description |
|--------|-------------|
| `4`    | The `level` |
| `4`    | The `round` |

### `GIT`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x09` | `__` | `__` |

Get the commit hash.

#### Input data

No input data.

#### Output data

| Length       | Description  |
|--------------|--------------|
| `<variable>` | The `commit` |

### `SETUP`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x0a` | `0x00` | `P2` |

Requests:
 - authorization to bake with the key associated with the given `path`
   and `P2`
 - a reset of the minimum level authorised in the main chain by
   `main-level`.
 - a reset of the minimum level authorised in the test chains by
   `test-level`.
 - to set the main chain id to `chain_id`.

Once accepted:
 - the key will be defined as the [`authorized-key`](NVRAM.md#authorized-key)
 - the maintened [`chain-id`](NVRAM.md#chain-id) will be set to `chain_id`.
 - the main [`HWM`](NVRAM.md#hwm) level will be set to `main-level` and its round will
   be set to `0`.
 - the test [`HWM`](NVRAM.md#hwm) level will be set to `test-level` and its round will
   be set to `0`.
 - the public key is returned.

#### Input data

| Length       | Description      |
|--------------|------------------|
| `4`          | The `chain_id`   |
| `4`          | The `main-level` |
| `4`          | The `test-level` |
| `<variable>` | The `path`       |

#### Output data

| Length     | Description             |
|------------|-------------------------|
| `1`        | The public key `length` |
| `<length>` | The public key          |

### `QUERY_ALL_HWM`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x0b` | `__` | `__` |

Get:
 - the [`HWM`](NVRAM.md#hwm) of the main chain.
 - the [`HWM`](NVRAM.md#hwm) of the test chains.
 - the main [`chain-id`](NVRAM.md#chain-id).

#### Input data

No input data.

#### Output data

| Length | Description      |
|--------|------------------|
| `4`    | The main `level` |
| `4`    | The main `round` |
| `4`    | The test `level` |
| `4`    | The test `round` |
| `4`    | The `chain_id`   |

### `DEAUTHORIZE`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x0c` | `0x00` | `__` |

Deauthorize the [`authorized-key`](NVRAM.md#authorized-key).

#### Input data

No input data expected.

#### Output data

No output data.

### `QUERY_AUTH_KEY_WITH_CURVE`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x0d` | `__` | `__` |

Get the path and the curve of the [`authorized-key`](NVRAM.md#authorized-key).

#### Input data

No input data.

#### Output data

| Length       | Description |
|--------------|-------------|
| `1`          | The `curve` |
| `<variable>` | The `path`  |

### `HMAC`

| *CLA*  | *INS*  | *P1*   | *P2* |
|--------|--------|--------|------|
| `0x80` | `0x0e` | `0x00` | `P2` |

Get the HMAC of the `message` produced using as key the signature of a
fixed message signed by the key associated with the `path` and `P2`.

#### Input data

| Length       | Description   |
|--------------|---------------|
| `<variable>` | The `path`    |
| `<variable>` | The `message` |

#### Output data

| Length       | Description |
|--------------|-------------|
| `<variable>` | The `hmac`  |

### `SIGN_WITH_HASH`

| *CLA*  | *INS*  | *P1* | *P2* |
|--------|--------|------|------|
| `0x80` | `0x0e` | `__` | `__` |

Runs in the same way as `SIGN` except that the value returned, when *P1* is `0x01` or `0x81`, also contains the hash of the signed operation.

#### Output data

| Length       | Description   |
|--------------|---------------|
| `32`         | The hash      |
| `<variable>` | The signature |
