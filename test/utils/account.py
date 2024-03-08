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

"""Module providing an account interface."""

from enum import IntEnum
from typing import Union

import base58
import pysodium
import secp256k1
import fastecdsa

import pytezos
from bip_utils.bip.bip32.bip32_path import Bip32Path, Bip32PathParser
from bip_utils.bip.bip32.bip32_key_data import Bip32KeyIndex
from utils.helper import BytesReader

class SigScheme(IntEnum):
    """Class representing signature scheme."""

    ED25519       = 0x00
    SECP256K1     = 0x01
    SECP256R1     = 0x02
    BIP32_ED25519 = 0x03
    DEFAULT       = ED25519

class BipPath:
    """Class representing mnemonic path."""
    value: Bip32Path

    def __init__(self, value: Bip32Path):
        self.value = value

    def __eq__(self, other: object):
        if not isinstance(other, BipPath):
            return NotImplemented
        return \
            self.value.m_elems == other.value.m_elems and \
            self.value.m_is_absolute == other.value.m_is_absolute

    def __bytes__(self) -> bytes:
        res = self.value.Length().to_bytes(1, byteorder='big')
        for elem in self.value:
            res += bytes(elem)
        return res

    def __repr__(self) -> str:
        return str(self.value)

    @classmethod
    def from_string(cls, path: str) -> 'BipPath':
        """Return the path from its string representation."""
        return BipPath(Bip32PathParser.Parse(path))

    @classmethod
    def from_bytes(cls, raw_path: bytes) -> 'BipPath':
        """Create a path from bytes."""
        reader = BytesReader(raw_path)
        path_len = reader.read_int(1)
        elems = []
        for _ in range(path_len):
            elems.append(
                Bip32KeyIndex.FromBytes(
                    reader.read_bytes(Bip32KeyIndex.FixedLength())
                ))
        return BipPath(Bip32Path(elems))

class Signature:
    """Class representing signature."""

    GENERIC_SIGNATURE_PREFIX = bytes.fromhex("04822b") # sig(96)

    def __init__(self, value: bytes):
        value = Signature.GENERIC_SIGNATURE_PREFIX + value
        self.value: bytes = base58.b58encode_check(value)

    def __repr__(self) -> str:
        return self.value.hex()

    @classmethod
    def from_tlv(cls, tlv: Union[bytes, bytearray]) -> 'Signature':
        """Get the signature encapsulated in a TLV."""
        # See:
        # https://developers.ledger.com/docs/embedded-app/crypto-api/lcx__ecdsa_8h/#cx_ecdsa_sign
        # TLV: 30 || L || 02 || Lr || r || 02 || Ls || s
        if isinstance(tlv, bytes):
            tlv = bytearray(tlv)
        header_tag_index = 0
        # Remove the unwanted parity information set here.
        tlv[header_tag_index] &= ~0x01
        if tlv[header_tag_index] != 0x30:
            raise ValueError("Invalid TLV tag")
        len_index = 1
        if tlv[len_index] != len(tlv) - 2:
            raise ValueError("Invalid TLV length")
        first_tag_index = 2
        if tlv[first_tag_index] != 0x02:
            raise ValueError("Invalid TLV tag")
        r_len_index = 3
        r_index = 4
        r_len = tlv[r_len_index]
        second_tag_index = r_index + r_len
        if tlv[second_tag_index] != 0x02:
            raise ValueError("Invalid TLV tag")
        s_len_index = second_tag_index + 1
        s_index = s_len_index + 1
        s_len = tlv[s_len_index]
        r = tlv[r_index : r_index + r_len]
        s = tlv[s_index : s_index + s_len]
        # Sometimes \x00 are added or removed
        # A size adjustment is required here.
        def adjust_size(data, size):
            return data[-size:].rjust(size, b'\x00')
        return Signature(adjust_size(r, 32) + adjust_size(s, 32))

    @classmethod
    def from_bytes(cls, data: bytes, sig_scheme: SigScheme) -> 'Signature':
        """Get the signature according to the SigScheme."""
        if sig_scheme in { SigScheme.ED25519, SigScheme.BIP32_ED25519 }:
            return Signature(data)
        return Signature.from_tlv(data)

class Account:
    """Class representing account."""

    def __init__(self,
                 path: Union[BipPath, str, bytes],
                 sig_scheme: SigScheme,
                 key: str):
        self.path: BipPath = \
            BipPath.from_string(path) if isinstance(path, str) else \
            BipPath.from_bytes(path) if isinstance(path, bytes) else \
            path
        self.sig_scheme: SigScheme = sig_scheme
        self.key: pytezos.Key = pytezos.pytezos.using(key=key).key

    @property
    def public_key_hash(self) -> str:
        """public_key_hash of the account."""
        return self.key.public_key_hash()

    @property
    def public_key(self) -> str:
        """public_key of the account."""
        return self.key.public_key()

    @property
    def secret_key(self) -> str:
        """secret_key of the account."""
        return self.key.secret_key()

    def __repr__(self) -> str:
        return f"{self.sig_scheme.name}_{self.public_key_hash}"

    def sign(self, message: Union[str, bytes], generic: bool = False) -> bytes:
        """Sign a raw sequence of bytes."""
        return self.key.sign(message, generic)

    def sign_prehashed_message(self, prehashed_message: bytes) -> bytes:
        """Sign a raw sequence of bytes already hashed."""
        if self.sig_scheme in [
                SigScheme.ED25519,
                SigScheme.BIP32_ED25519
        ]:
            return pysodium.crypto_sign_detached(
                prehashed_message,
                self.key.secret_exponent
            )
        if self.sig_scheme == SigScheme.SECP256K1:
            pk = secp256k1.PrivateKey(self.key.secret_exponent)
            return pk.ecdsa_serialize_compact(
                pk.ecdsa_sign(
                    prehashed_message,
                    raw=True
                )
            )
        if self.sig_scheme == SigScheme.SECP256R1:
            r, s = fastecdsa.ecdsa.sign(
                msg=prehashed_message,
                d=fastecdsa.encoding.util.bytes_to_int(self.key.secret_exponent),
                prehashed=True
            )
            return r.to_bytes(32, 'big') + s.to_bytes(32, 'big')
        raise ValueError(f"Account do not have a right signature type: {self.sig_scheme}")

    @property
    def base58_decoded(self) -> bytes:
        """base58_decoded of the account."""

        # Get the public_key without prefix
        public_key = base58.b58decode_check(self.public_key)

        if self.sig_scheme in [
                SigScheme.ED25519,
                SigScheme.BIP32_ED25519
        ]:
            prefix = bytes.fromhex("0d0f25d9") # edpk(54)
        elif self.sig_scheme == SigScheme.SECP256K1:
            prefix = bytes.fromhex("03fee256") # sppk(55)
        elif self.sig_scheme == SigScheme.SECP256R1:
            prefix = bytes.fromhex("03b28b7f") # p2pk(55)
        else:
            raise ValueError(f"Account do not have a right signature type: {self.sig_scheme}")
        assert public_key.startswith(prefix), \
            "Expected prefix {prefix.hex()} but got {public_key.hex()}"

        public_key = public_key[len(prefix):]

        if self.sig_scheme in [
                SigScheme.SECP256K1,
                SigScheme.SECP256R1
        ]:
            assert public_key[0] in [0x02, 0x03], \
                "Expected a prefix kind of 0x02 or 0x03 but got {public_key[0]}"
            public_key = public_key[1:]

        return public_key

    def check_public_key(self, data: bytes) -> None:
        """Check that the data correspond to the account."""

        # `data` should be:
        # length + kind + pk
        # kind : 02=odd, 03=even, 04=uncompressed
        # pk length = 32 for compressed, 64 for uncompressed
        assert len(data) - 1 == data[0], \
                "Expected a length of {data[0]} but got {len(data) - 1}"
        if data[1] == 0x04: # public key uncompressed
            assert data[0] == 1 + 32 + 32, \
                "Expected a length of 1 + 32 + 32 but got {data[0]}"
        elif data[1] in [0x02, 0x03]: # public key even or odd (compressed)
            assert data[0] == 1 + 32, \
                "Expected a length of 1 + 32 but got {data[0]}"
        else:
            raise ValueError(f"Expected a prefix kind of 0x02, 0x03 or 0x04 but got {data[1]}")
        data = data[2:2+32]

        public_key = self.base58_decoded
        assert data == public_key, \
            f"Expected public key {public_key.hex()} but got {data.hex()}"

    def check_signature(self,
                        signature: Union[bytes, Signature],
                        message: Union[str, bytes]):
        """Check that the signature is the signature of the message by the account."""
        if isinstance(message, str):
            message = bytes.fromhex(message)
        if isinstance(signature, bytes):
            signature = Signature.from_bytes(signature, self.sig_scheme)
        assert self.key.verify(signature.value, message), \
            f"Fail to verify signature {signature}, \n\
            with account {self} \n\
            and message {message.hex()}"
