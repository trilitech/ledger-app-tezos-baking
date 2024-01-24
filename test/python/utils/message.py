# Copyright 2023 Functori <contact@functori.com>

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Implemenation of sent messages."""

from abc import ABC, abstractmethod
from enum import IntEnum
from hashlib import blake2b
from typing import Union

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

def scrub(value: Union[str, bytes]) -> bytes:
    """Helping function to handle str as bytes."""
    return value if isinstance(value, bytes) else \
        bytes.fromhex(value)

def assert_data_size(name: str, data: bytes, size: int):
    assert len(data) == size, \
        f"Wrong {name} size: {data.hex()} should be {size}-bytes long."

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
    TENDERBAKE_PREENDORSEMENT = 0x12
    TENDERBAKE_ENDORSEMENT    = 0x13

class OperationTag(IntEnum):
    """Class representing the operation tag."""

    PROPOSAL            = 5
    BALLOT              = 6
    BABYLON_REVEAL      = 107
    BABYLON_TRANSACTION = 108
    BABYLON_ORIGINATION = 109
    BABYLON_DELEGATION  = 110
    ATHENS_REVEAL       = 7
    ATHENS_TRANSACTION  = 8
    ATHENS_ORIGINATION  = 9
    ATHENS_DELEGATION   = 10
    PREATTESTATION      = 20

class Preattestation(Message):
    """Class representing a preattestation."""
    chain_id: int
    branch: bytes
    slot: int
    op_level: int
    op_round: int
    block_payload_hash: bytes

    def __init__(self,
                 chain_id: int,
                 branch: Union[str, bytes],
                 slot: int,
                 op_level: int,
                 op_round: int,
                 block_payload_hash: Union[str, bytes]):
        self.chain_id = chain_id
        self.branch = scrub(branch)
        assert_data_size("branch", self.branch, 32)
        self.slot = slot
        self.op_level = op_level
        self.op_round = op_round
        self.block_payload_hash = scrub(block_payload_hash)
        assert_data_size("block_payload_hash", self.block_payload_hash, 32)

    def raw(self) -> bytes:
        raw = b''
        raw += MagicByte.TENDERBAKE_PREENDORSEMENT.to_bytes(1, byteorder='big')
        raw += self.chain_id.to_bytes(4, byteorder='big')
        raw += self.branch
        raw += OperationTag.PREATTESTATION.to_bytes(1, byteorder='big')
        raw += self.slot.to_bytes(2, byteorder='big')
        raw += self.op_level.to_bytes(4, byteorder='big')
        raw += self.op_round.to_bytes(4, byteorder='big')
        raw += self.block_payload_hash
        return raw
