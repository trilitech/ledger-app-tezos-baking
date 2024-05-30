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
from hashlib import blake2b
from typing import Optional, Union
from pytezos import pytezos
from pytezos.operation.group import OperationGroup
from pytezos.block.forge import forge_int_fixed, forge_fitness
from pytezos.michelson import forge

class Message(ABC):
    """Class representing a message."""

    HASH_SIZE = 32

    @property
    def hash(self) -> bytes:
        """hash of the message."""
        return blake2b(
            self.raw(),
            digest_size=Message.HASH_SIZE
        ).digest()

    @abstractmethod
    def raw(self) -> bytes:
        """bytes representation of the message."""
        raise NotImplementedError

    def __bytes__(self) -> bytes:
        return self.raw()

class RawMessage(Message):
    """Class representing a raw message."""

    def __init__(self, value: Union[str, bytes]):
        self.value: bytes = value if isinstance(value, bytes) else \
            bytes.fromhex(value)

    def raw(self) -> bytes:
        return self.value

class MagicByte(IntEnum):
    """Class representing the magic byte."""

    INVALID                   = 0x00
    BLOCK                     = 0x01
    BAKING_OP                 = 0x02
    UNSAFE_OP                 = 0x03
    UNSAFE_OP2                = 0x04
    UNSAFE_OP3                = 0x05
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

# Chain_id.zero
DEFAULT_CHAIN_ID = "NetXH12Aer3be93"
# Block_hash.zero
DEFAULT_BLOCK_HASH = "BKiHLREqU3JkXfzEDYAkmmfX48gBDtYhMrpA98s7Aq4SzbUAB6M"
# Block_payload_hash.zero
DEFAULT_BLOCK_PAYLOAD_HASH = "vh1g87ZG6scSYxKhspAUzprQVuLAyoa5qMBKcUfjgnQGnFb3dJcG"
# Operation_list_list_hash.zero
DEFAULT_OPERATIONS_HASH = "LLoZKi1iMzbeJrfrGWPFYmkLebcsha6vGskQ4rAXt2uMwQtBfRcjL"
# Time.Protocol.epoch
DEFAULT_TIMESTAMP = "1970-01-01T00:00:00-00:00"
# Context_hash.zero
DEFAULT_CONTEXT_HASH = "CoUeJrcPBj3T3iJL3PY4jZHnmZa5rRZ87VQPdSBNBcwZRMWJGh9j"

class UnsafeOp:
    """Class representing an unsafe operation."""

    operation: OperationGroup

    def __init__(self, operation: OperationGroup):
        self.operation = operation

    def forge(self, branch: str = DEFAULT_BLOCK_HASH) -> Message:
        """Forge the operation."""
        watermark = forge_int_fixed(MagicByte.UNSAFE_OP, 1)
        self.operation.branch = branch
        raw = watermark + bytes.fromhex(self.operation.forge())
        return RawMessage(raw)

    def merge(self, unsafe_op: 'UnsafeOp') -> 'UnsafeOp':
        res = self.operation
        for content in unsafe_op.operation.contents:
            res = res.operation(content)
        return UnsafeOp(res)

class Delegation(UnsafeOp):
    """Class representing a delegation."""

    def __init__(self,
                 delegate: str,
                 source: str,
                 counter: int = 0,
                 fee: int = 0,
                 gas_limit: int = 0,
                 storage_limit: int = 0):
        ctxt = pytezos.using()
        delegation = ctxt.delegation(delegate, source, counter, fee, gas_limit, storage_limit)
        super().__init__(delegation)

class Reveal(UnsafeOp):
    """Class representing a reveal."""

    def __init__(self,
                 public_key: str,
                 source: str,
                 counter: int = 0,
                 fee: int = 0,
                 gas_limit: int = 0,
                 storage_limit: int = 0):
        ctxt = pytezos.using()
        reveal = ctxt.reveal(public_key, source, counter, fee, gas_limit, storage_limit)
        super().__init__(reveal)

class Preattestation:
    """Class representing a preattestation."""

    slot: int
    op_level: int
    op_round: int
    block_payload_hash: str

    def __init__(self,
                 slot: int = 0,
                 op_level: int = 0,
                 op_round: int = 0,
                 block_payload_hash: str = DEFAULT_BLOCK_PAYLOAD_HASH):
        self.slot               = slot
        self.op_level           = op_level
        self.op_round           = op_round
        self.block_payload_hash = block_payload_hash

    def __bytes__(self) -> bytes:
        raw = b''
        raw += forge_int_fixed(OperationTag.PREATTESTATION, 1)
        raw += forge.forge_int16(self.slot)
        raw += forge.forge_int32(self.op_level)
        raw += forge.forge_int32(self.op_round)
        raw += forge.forge_base58(self.block_payload_hash)
        return raw

    def forge(self,
              chain_id: str = DEFAULT_CHAIN_ID,
              branch: str = DEFAULT_BLOCK_HASH) -> Message:
        """Forge the preattestation."""
        raw_operation = \
            forge.forge_base58(branch) + \
            bytes(self)
        watermark = \
            forge_int_fixed(MagicByte.TENDERBAKE_PREATTESTATION, 1) + \
            forge.forge_base58(chain_id)
        raw = watermark + raw_operation
        return RawMessage(raw)

class Attestation:
    """Class representing an attestation."""

    slot: int
    op_level: int
    op_round: int
    block_payload_hash: str

    def __init__(self,
                 slot: int = 0,
                 op_level: int = 0,
                 op_round: int = 0,
                 block_payload_hash: str = DEFAULT_BLOCK_PAYLOAD_HASH):
        self.slot               = slot
        self.op_level           = op_level
        self.op_round           = op_round
        self.block_payload_hash = block_payload_hash

    def __bytes__(self) -> bytes:
        raw = b''
        raw += forge_int_fixed(OperationTag.ATTESTATION, 1)
        raw += forge.forge_int16(self.slot)
        raw += forge.forge_int32(self.op_level)
        raw += forge.forge_int32(self.op_round)
        raw += forge.forge_base58(self.block_payload_hash)
        return raw

    def forge(self,
              chain_id: str = DEFAULT_CHAIN_ID,
              branch: str = DEFAULT_BLOCK_HASH) -> Message:
        """Forge the attestation."""
        raw_operation = \
            forge.forge_base58(branch) + \
            bytes(self)
        watermark = \
            forge_int_fixed(MagicByte.TENDERBAKE_ATTESTATION, 1) + \
            forge.forge_base58(chain_id)
        raw = watermark + raw_operation
        return RawMessage(raw)

class AttestationDal:
    """Class representing an attestation + DAL."""

    slot: int
    op_level: int
    op_round: int
    block_payload_hash: str
    dal_attestation: int

    def __init__(self,
                 slot: int = 0,
                 op_level: int = 0,
                 op_round: int = 0,
                 block_payload_hash: str = DEFAULT_BLOCK_PAYLOAD_HASH,
                 dal_attestation: int = 0):
        self.slot               = slot
        self.op_level           = op_level
        self.op_round           = op_round
        self.block_payload_hash = block_payload_hash
        self.dal_attestation    = dal_attestation

    def __bytes__(self) -> bytes:
        raw = b''
        raw += forge_int_fixed(OperationTag.ATTESTATION_WITH_DAL, 1)
        raw += forge.forge_int16(self.slot)
        raw += forge.forge_int32(self.op_level)
        raw += forge.forge_int32(self.op_round)
        raw += forge.forge_base58(self.block_payload_hash)
        raw += forge.forge_nat(self.dal_attestation)
        return raw

    def forge(self,
              chain_id: str = DEFAULT_CHAIN_ID,
              branch: str = DEFAULT_BLOCK_HASH) -> Message:
        """Forge the attestation + DAL."""
        raw_operation = \
            forge.forge_base58(branch) + \
            bytes(self)
        watermark = \
            forge_int_fixed(MagicByte.TENDERBAKE_ATTESTATION, 1) + \
            forge.forge_base58(chain_id)
        raw = watermark + raw_operation
        return RawMessage(raw)

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

    def __bytes__(self) -> bytes:
        raw_locked_round = \
            b'' if self.locked_round is None else \
            forge.forge_int32(self.locked_round)
        raw_predecessor_round = \
            (-self.predecessor_round-1).to_bytes(4, 'big', signed=True)
        return forge_fitness(
            [
                forge_int_fixed(ConsensusProtocol.TENDERBAKE, 1).hex(),
                forge.forge_int32(self.level).hex(),
                raw_locked_round.hex(),
                raw_predecessor_round.hex(),
                forge.forge_int32(self.current_round).hex()
            ]
        )

class BlockHeader:
    """Class representing a block header."""

    level: int
    proto_level: int
    predecessor: str
    timestamp: str
    validation_pass: int
    operations_hash: str
    fitness: Fitness
    context: str

    def __init__(self,
                 level: int = 0,
                 proto_level: int = 0,
                 predecessor: str = DEFAULT_BLOCK_HASH,
                 timestamp: str = DEFAULT_TIMESTAMP,
                 validation_pass: int = 0,
                 operations_hash: str = DEFAULT_OPERATIONS_HASH,
                 fitness: Fitness = Fitness(),
                 context: str = DEFAULT_CONTEXT_HASH):
        self.level           = level
        self.proto_level     = proto_level
        self.predecessor     = predecessor
        self.timestamp       = timestamp
        self.validation_pass = validation_pass
        self.operations_hash = operations_hash
        self.fitness         = fitness
        self.context         = context

    def __bytes__(self) -> bytes:
        raw = b''
        raw += forge_int_fixed(self.level, 4)
        raw += forge_int_fixed(self.proto_level, 1)
        raw += forge.forge_base58(self.predecessor)
        raw += forge_int_fixed(forge.optimize_timestamp(self.timestamp), 8)
        raw += forge_int_fixed(self.validation_pass, 1)
        raw += forge.forge_base58(self.operations_hash)
        raw += bytes(self.fitness)
        raw += forge.forge_base58(self.context)
        return raw

class Block:
    """Class representing a block."""

    header: BlockHeader
    content: bytes

    def __init__(self,
                 header: BlockHeader = BlockHeader(),
                 content: Union[str, bytes] = b''):
        self.header  = header
        self.content = content if isinstance(content, bytes) else \
            bytes.fromhex(content)

    def __bytes__(self) -> bytes:
        return bytes(self.header) + self.content

    def forge(self, chain_id: str = DEFAULT_CHAIN_ID) -> Message:
        """Forge the block."""
        watermark = \
            forge_int_fixed(MagicByte.TENDERBAKE_BLOCK, 1) + \
            forge.forge_base58(chain_id)
        raw = watermark + bytes(self)
        return RawMessage(raw)
