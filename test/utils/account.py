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
from pytezos.crypto.encoding import base58_encode
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

    @staticmethod
    def from_secp256_tlv(tlv: Union[bytes, bytearray]) -> bytes:
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
        return adjust_size(r, 32) + adjust_size(s, 32)

    @classmethod
    def from_bytes(cls, data: bytes, sig_scheme: SigScheme) -> str:
        """Get the signature according to the SigScheme."""
        if sig_scheme in { SigScheme.ED25519, SigScheme.BIP32_ED25519 }:
            prefix = bytes([9, 245, 205, 134, 18])
        elif sig_scheme in { SigScheme.SECP256K1, SigScheme.SECP256R1 }:
            prefix = bytes([13, 115, 101, 19, 63]) if sig_scheme == SigScheme.SECP256K1 \
                else bytes([54, 240, 44, 52])
            data = Signature.from_secp256_tlv(data)
        else:
            assert False, f"Wrong signature type: {sig_scheme}"

        return base58.b58encode_check(prefix + data).decode()

class PublicKey:
    """Set of functions over public key management"""

    class CompressionKind(IntEnum):
        """Bytes compression kind"""
        EVEN         = 0x02
        ODD          = 0x03
        UNCOMPRESSED = 0x04

        def __bytes__(self) -> bytes:
            return bytes([self])

    @staticmethod
    def from_bytes(data: bytes, sig_scheme: SigScheme) -> str:
        """Convert a public key from bytes to string"""
        # `data` should be:
        # kind + pk
        # pk length = 32 for compressed, 64 for uncompressed
        kind = data[0]
        data = data[1:]

        # Ed25519
        if sig_scheme in [
                SigScheme.ED25519,
                SigScheme.BIP32_ED25519
        ]:
            assert kind == PublicKey.CompressionKind.EVEN, \
                f"Wrong Ed25519 public key compression kind: {kind}"
            assert len(data) == 32, \
                f"Wrong Ed25519 public key length: {len(data)}"
            return base58_encode(data, b'edpk').decode()

        # Secp256
        if sig_scheme in [
                SigScheme.SECP256K1,
                SigScheme.SECP256R1
        ]:
            assert kind == PublicKey.CompressionKind.UNCOMPRESSED, \
                f"Wrong Secp256 public key compression kind: {kind}"
            assert len(data) == 2 * 32, \
                f"Wrong Secp256 public key length: {len(data)}"
            kind = PublicKey.CompressionKind.ODD if data[-1] & 1 else \
                PublicKey.CompressionKind.EVEN
            prefix = b'sppk' if sig_scheme == SigScheme.SECP256K1 \
                else b'p2pk'
            data = bytes(kind) + data[:32]
            return base58_encode(data, prefix).decode()

        assert False, f"Wrong signature type: {sig_scheme}"

class Account:
    """Class representing account."""

    def __init__(self,
                 path: Union[BipPath, str, bytes],
                 sig_scheme: SigScheme,
                 key: str,
                 nanos_screens: int):
        self.path: BipPath = \
            BipPath.from_string(path) if isinstance(path, str) else \
            BipPath.from_bytes(path) if isinstance(path, bytes) else \
            path
        self.sig_scheme: SigScheme = sig_scheme
        self.key: pytezos.Key = pytezos.pytezos.using(key=key).key
        self.nanos_screens: int = nanos_screens

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

    def check_signature(self,
                        signature: Union[str, bytes],
                        message: Union[str, bytes]):
        """Check that the signature is the signature of the message by the account."""
        if isinstance(message, str):
            message = bytes.fromhex(message)
        if isinstance(signature, bytes):
            signature = Signature.from_bytes(signature, self.sig_scheme)
        assert self.key.verify(signature.encode(), message), \
            f"Fail to verify signature {signature}, \n\
            with account {self} \n\
            and message {message.hex()}"
