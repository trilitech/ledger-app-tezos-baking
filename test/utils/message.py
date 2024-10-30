# Copyright 2024 Functori <contact@functori.com>
# Copyright 2024 Trilitech <contact@trili.tech>

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Implemenation of sent messages."""

from abc import ABC, abstractmethod
from enum import IntEnum
from typing import Any, Dict, List, Optional, Union

import base58

from pytezos.block.forge import forge_block_header, forge_int_fixed
from pytezos.crypto.encoding import base58_encodings
from pytezos.crypto.key import blake2b_32
from pytezos.michelson.forge import (
    forge_address,
    forge_base58,
    forge_int16,
    forge_int32,
    forge_nat,
    # forge_public_key, # overrode until BLS key included
)
from pytezos.operation.content import ContentMixin
from pytezos.operation.forge import forge_tag
# from pytezos.operation.group import OperationGroup
from pytezos.rpc.kind import operation_tags

class Message(ABC):
    """Class representing a message."""

    HASH_SIZE = 32

    @property
    def hash(self) -> bytes:
        """hash of the message."""
        return blake2b_32(bytes(self)).digest()

    @abstractmethod
    def __bytes__(self) -> bytes:
        raise NotImplementedError

class RawMessage(Message):
    """Class representing a raw message."""

    _value: bytes

    def __init__(self, value: Union[str, bytes]):
        self._value = value if isinstance(value, bytes) else \
            bytes.fromhex(value)

    def __bytes__(self) -> bytes:
        return self._value

class Watermark(IntEnum):
    """Class hodling messages watermark."""

    INVALID                   = 0x00
    BLOCK                     = 0x01
    BAKING_OP                 = 0x02
    MANAGER_OPERATION         = 0x03
    UNSAFE_OP2                = 0x04
    MICHELINE_EXPRESSION      = 0x05
    TENDERBAKE_BLOCK          = 0x11
    TENDERBAKE_PREATTESTATION = 0x12
    TENDERBAKE_ATTESTATION    = 0x13

class ConsensusProtocol(IntEnum):
    """Class representing the consensus protocol."""

    EMMY       = 0
    EMMY_PLUS  = 1
    TENDERBAKE = 2

class OperationTag(IntEnum):
    """Class representing the operation tag."""

    PROPOSAL             = 5
    BALLOT               = 6
    BABYLON_REVEAL       = 107
    BABYLON_TRANSACTION  = 108
    BABYLON_ORIGINATION  = 109
    BABYLON_DELEGATION   = 110
    ATHENS_REVEAL        = 7
    ATHENS_TRANSACTION   = 8
    ATHENS_ORIGINATION   = 9
    ATHENS_DELEGATION    = 10
    PREATTESTATION       = 20
    ATTESTATION          = 21
    ATTESTATION_WITH_DAL = 23

Micheline = Union[List, Dict]

class Default:
    """Class holding default values."""

    BLOCK_HASH: str              = "BKiHLREqU3JkXfzEDYAkmmfX48gBDtYhMrpA98s7Aq4SzbUAB6M"
    BLOCK_PAYLOAD_HASH: str      = "vh1g87ZG6scSYxKhspAUzprQVuLAyoa5qMBKcUfjgnQGnFb3dJcG"
    CHAIN_ID: str                = "NetXH12Aer3be93"
    CONTEXT_HASH: str            = "CoUeJrcPBj3T3iJL3PY4jZHnmZa5rRZ87VQPdSBNBcwZRMWJGh9j"
    ED25519_PUBLIC_KEY: str      = 'edpkteDwHwoNPB18tKToFKeSCykvr1ExnoMV5nawTJy9Y9nLTfQ541'
    ED25519_PUBLIC_KEY_HASH: str = 'tz1Ke2h7sDdakHJQh8WX4Z372du1KChsksyU'
    ENTRYPOINT: str              = 'default'
    OPERATIONS_HASH: str         = "LLoZKi1iMzbeJrfrGWPFYmkLebcsha6vGskQ4rAXt2uMwQtBfRcjL"
    TIMESTAMP: str               = "1970-01-01T00:00:00-00:00"

    class Micheline:
        """Class holding Micheline default values."""
        VALUE: Micheline = {'prim': 'Unit'}

class OperationBuilder(ContentMixin):
    """Extends and fix pytezos.operation.content.ContentMixin."""

    def preattestation(
            self,
            slot: int = 0,
            op_level: int = 0,
            op_round: int = 0,
            block_payload_hash: str = Default.BLOCK_PAYLOAD_HASH):
        """Build a tezos preattestation."""
        return self.operation(
            {
                'kind': 'preattestation',
                'slot': slot,
                'level': op_level,
                'round': op_round,
                'block_payload_hash': block_payload_hash,
            }
        )

    def attestation(
            self,
            slot: int = 0,
            op_level: int = 0,
            op_round: int = 0,
            block_payload_hash: str = Default.BLOCK_PAYLOAD_HASH,
            dal_attestation: Optional[int] = None):
        """Build a tezos attestation."""
        kind = 'attestation' if dal_attestation is None \
            else 'attestation_with_dal'
        content = {
            'kind': kind,
            'slot': slot,
            'level': op_level,
            'round': op_round,
            'block_payload_hash': block_payload_hash,
        }

        if dal_attestation is not None:
            content['dal_attestation'] = str(dal_attestation)

        return self.operation(content)

base58_encodings += [(b"BLpk", 76, bytes([6, 149, 135, 204]), 48, "BLS public key")]

def forge_public_key(value: str) -> bytes:
    """Encode public key into bytes.

    :param value: public key in in base58 form
    """
    prefix = value[:4]
    res = base58.b58decode_check(value)[4:]

    if prefix == 'edpk':  # noqa: SIM116
        return b'\x00' + res
    if prefix == 'sppk':
        return b'\x01' + res
    if prefix == 'p2pk':
        return b'\x02' + res
    if prefix == 'BLpk':
        return b'\x03' + res

    raise ValueError(f'Unrecognized key type: #{prefix}')

class OperationForge:
    """Extends and fix pytezos.operation.forge."""
    import pytezos.operation.forge as operation

    operation_tags['preattestation'] = 20
    operation_tags['attestation'] = 21
    operation_tags['attestation_with_dal'] = 23

    delegation = operation.forge_delegation
    transaction = operation.forge_transaction

    @staticmethod
    def reveal(content: Dict[str, Any]) -> bytes:
        """Forge a tezos reveal."""
        res = forge_tag(operation_tags[content['kind']])
        res += forge_address(content['source'], tz_only=True)
        res += forge_nat(int(content['fee']))
        res += forge_nat(int(content['counter']))
        res += forge_nat(int(content['gas_limit']))
        res += forge_nat(int(content['storage_limit']))
        res += forge_public_key(content['public_key'])
        return res

    @staticmethod
    def preattestation(content: Dict[str, Any]) -> bytes:
        """Forge a tezos preattestation."""
        res = forge_tag(operation_tags[content['kind']])
        res += forge_int16(content['slot'])
        res += forge_int32(content['level'])
        res += forge_int32(content['round'])
        res += forge_base58(content['block_payload_hash'])
        return res

    @staticmethod
    def attestation(content: Dict[str, Any]) -> bytes:
        """Forge a tezos attestation."""
        res = forge_tag(operation_tags[content['kind']])
        res += forge_int16(content['slot'])
        res += forge_int32(content['level'])
        res += forge_int32(content['round'])
        res += forge_base58(content['block_payload_hash'])
        if content.get('dal_attestation'):
            res += forge_nat(int(content['dal_attestation']))
        return res

class Operation(Message, OperationBuilder):
    """Class representing a tezos operation."""

    branch: str

    def __init__(self, branch: str = Default.BLOCK_HASH):
        self.branch = branch

    @abstractmethod
    def forge(self) -> bytes:
        """Forge the operation."""
        raise NotImplementedError

    @property
    @abstractmethod
    def watermark(self) -> bytes:
        """Watermark of the operation."""
        raise NotImplementedError

    def __bytes__(self) -> bytes:
        raw = b''
        raw += self.watermark
        raw += forge_base58(self.branch)
        raw += self.forge()
        return raw

class ManagerOperation(Operation):
    """Class representing a tezos manager operation."""

    source: str
    fee: int
    counter: int
    gas_limit: int
    storage_limit: int

    @property
    def watermark(self) -> bytes:
        return forge_int_fixed(Watermark.MANAGER_OPERATION, 1)

    def __init__(self,
                 source: str = Default.ED25519_PUBLIC_KEY_HASH,
                 fee: int = 0,
                 counter: int = 0,
                 gas_limit: int = 0,
                 storage_limit: int = 0,
                 *args, **kwargs):
        self.source = source
        self.fee = fee
        self.counter = counter
        self.gas_limit = gas_limit
        self.storage_limit = storage_limit
        Operation.__init__(self, *args, **kwargs)

class OperationGroup(Operation):
    """Class representing a group of tezos manager operation."""

    operations: List[ManagerOperation]

    @property
    def watermark(self) -> bytes:
        return forge_int_fixed(Watermark.MANAGER_OPERATION, 1)

    def __init__(self, operations: List[ManagerOperation], *args, **kwargs):
        self.operations = operations
        Operation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        return b''.join(map(lambda op: op.forge(), self.operations))

class Reveal(ManagerOperation):
    """Class representing a tezos reveal."""

    public_key: str

    def __init__(self,
                 public_key: str = Default.ED25519_PUBLIC_KEY,
                 *args, **kwargs):
        self.public_key = public_key
        ManagerOperation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        return OperationForge.reveal(
            self.reveal(
                self.public_key,
                self.source,
                self.counter,
                self.fee,
                self.gas_limit,
                self.storage_limit
            )
        )

class Delegation(ManagerOperation):
    """Class representing a tezos delegation."""

    delegate: Optional[str]

    def __init__(self,
                 delegate: Optional[str] = None,
                 *args, **kwargs):
        self.delegate = delegate
        ManagerOperation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        return OperationForge.delegation(
            self.delegation(
                self.delegate,
                self.source,
                self.counter,
                self.fee,
                self.gas_limit,
                self.storage_limit
            )
        )

class Transaction(ManagerOperation):
    """Class representing a tezos transaction."""

    destination: str
    amount: int
    entrypoint: str
    parameter: Micheline

    def __init__(self,
                 destination: str = Default.ED25519_PUBLIC_KEY_HASH,
                 amount: int = 0,
                 entrypoint: str = Default.ENTRYPOINT,
                 parameter: Micheline = Default.Micheline.VALUE,
                 *args, **kwargs):
        self.destination = destination
        self.amount = amount
        self.entrypoint = entrypoint
        self.parameter = parameter
        ManagerOperation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        parameters = { "entrypoint": self.entrypoint, "value": self.parameter }
        return OperationForge.transaction(
            self.transaction(
                self.destination,
                self.amount,
                parameters,
                self.source,
                self.counter,
                self.fee,
                self.gas_limit,
                self.storage_limit
            )
        )

class Preattestation(Operation):
    """Class representing a tezos preattestation."""

    slot: int
    op_level: int
    op_round: int
    block_payload_hash: str
    chain_id: str

    @property
    def watermark(self) -> bytes:
        return \
            forge_int_fixed(Watermark.TENDERBAKE_PREATTESTATION, 1) \
            + forge_base58(self.chain_id)

    def __init__(self,
                 slot: int = 0,
                 op_level: int = 0,
                 op_round: int = 0,
                 block_payload_hash: str = Default.BLOCK_PAYLOAD_HASH,
                 chain_id: str = Default.CHAIN_ID,
                 *args, **kwargs):
        self.slot = slot
        self.op_level = op_level
        self.op_round = op_round
        self.block_payload_hash = block_payload_hash
        self.chain_id = chain_id
        Operation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        return OperationForge.preattestation(
            self.preattestation(
                self.slot,
                self.op_level,
                self.op_round,
                self.block_payload_hash
            )
        )

class Attestation(Operation):
    """Class representing a tezos attestation."""

    slot: int
    op_level: int
    op_round: int
    block_payload_hash: str
    dal_attestation: Optional[int]
    chain_id: str

    @property
    def watermark(self) -> bytes:
        return \
            forge_int_fixed(Watermark.TENDERBAKE_ATTESTATION, 1) \
            + forge_base58(self.chain_id)

    def __init__(self,
                 slot: int = 0,
                 op_level: int = 0,
                 op_round: int = 0,
                 block_payload_hash: str = Default.BLOCK_PAYLOAD_HASH,
                 dal_attestation: Optional[int] = None,
                 chain_id: str = Default.CHAIN_ID,
                 *args, **kwargs):
        self.slot = slot
        self.op_level = op_level
        self.op_round = op_round
        self.block_payload_hash = block_payload_hash
        self.dal_attestation = dal_attestation
        self.chain_id = chain_id
        Operation.__init__(self, *args, **kwargs)

    def forge(self) -> bytes:
        return OperationForge.attestation(
            self.attestation(
                self.slot,
                self.op_level,
                self.op_round,
                self.block_payload_hash,
                self.dal_attestation
            )
        )

class Fitness:
    """Class representing a fitness."""

    level: int
    locked_round: Optional[int]
    predecessor_round: int
    current_round: int

    def __init__(self,
                 level: int = 0,
                 locked_round: Optional[int] = None,
                 predecessor_round: int = 0,
                 current_round: int = 0):
        self.level             = level
        self.locked_round      = locked_round
        self.predecessor_round = predecessor_round
        self.current_round     = current_round

    def build(self):
        """Build a tezos fitness."""
        raw_locked_round = \
            b'' if self.locked_round is None else \
            forge_int32(self.locked_round)
        raw_predecessor_round = \
            (-self.predecessor_round-1).to_bytes(4, 'big', signed=True)
        return [
            forge_int_fixed(ConsensusProtocol.TENDERBAKE, 1).hex(),
            forge_int32(self.level).hex(),
            raw_locked_round.hex(),
            raw_predecessor_round.hex(),
            forge_int32(self.current_round).hex()
        ]

class BlockHeader:
    """Class representing a block header."""

    level: int
    proto: int
    predecessor: str
    timestamp: str
    validation_pass: int
    operations_hash: str
    fitness: Fitness
    context: str
    protocol_data: str # Hex

    def __init__(self,
                 level: int = 0,
                 proto: int = 0,
                 predecessor: str = Default.BLOCK_HASH,
                 timestamp: str = Default.TIMESTAMP,
                 validation_pass: int = 0,
                 operations_hash: str = Default.OPERATIONS_HASH,
                 fitness: Fitness = Fitness(),
                 context: str = Default.CONTEXT_HASH,
                 protocol_data: str = ''):
        self.level = level
        self.proto = proto
        self.predecessor = predecessor
        self.timestamp = timestamp
        self.validation_pass = validation_pass
        self.operations_hash = operations_hash
        self.fitness = fitness
        self.context = context
        self.protocol_data = protocol_data

    def build(self):
        """Build a tezos Block header."""
        return {
            'level': self.level,
            'proto': self.proto,
            'predecessor': self.predecessor,
            'timestamp': self.timestamp,
            'validation_pass': self.validation_pass,
            'operations_hash': self.operations_hash,
            'fitness': self.fitness.build(),
            'context': self.context,
            'protocol_data': self.protocol_data,
        }

class Block(Message):
    """Class representing a block."""

    header: BlockHeader
    chain_id: str

    def __init__(self,
                 header: BlockHeader = BlockHeader(),
                 chain_id: str = Default.CHAIN_ID):
        self.header = header
        self.chain_id = chain_id

    def __bytes__(self) -> bytes:
        raw = b''
        raw += forge_int_fixed(Watermark.TENDERBAKE_BLOCK, 1)
        raw += forge_base58(self.chain_id)
        raw += forge_block_header(self.header.build())
        return raw
