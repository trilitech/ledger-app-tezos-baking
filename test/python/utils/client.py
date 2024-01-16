"""Module providing a tezos client."""

from typing import Generator, Optional
from enum import IntEnum
from contextlib import contextmanager

from ragger.utils import RAPDU
from ragger.backend import BackendInterface
from ragger.bip import pack_derivation_path

TEZ_PACKED_DERIVATION_PATH = pack_derivation_path("m/44'/1729'/0'/0'")
CLA = 0x80

CMD_PART1 = "17777d8de5596705f1cb35b0247b9605a7c93a7ed5c0caa454d4f4ff39eb411d"

CMD_PART2 = "00cf49f66b9ea137e11818f2a78b4b6fc9895b4e50830ae58003c35000c0843d0000eac6c762212c4110" \
    "f221ec8fcb05ce83db95845700"


class Ins(IntEnum):
    """Class representing instruction."""

    VERSION                   = 0x00
    GET_PUBLIC_KEY            = 0x02
    QUERY_AUTH_KEY            = 0x07
    QUERY_MAIN_HWM            = 0x08
    GIT                       = 0x09
    QUERY_ALL_HWM             = 0x0b
    DEAUTHORIZE               = 0x0c
    QUERY_AUTH_KEY_WITH_CURVE = 0x0d
    HMAC                      = 0x0e
    AUTHORIZE_BAKING          = 0x01
    PROMPT_PUBLIC_KEY         = 0x03
    SIGN                      = 0x04
    SIGN_UNSAFE               = 0x05
    RESET                     = 0x06
    SETUP                     = 0x0a
    SIGN_WITH_HASH            = 0x0f


class Index(IntEnum):
    """Class representing packet index."""

    FIRST = 0x00
    OTHER = 0x01
    LAST  = 0x81


class SigScheme(IntEnum):
    """Class representing signature scheme."""

    ED25519       = 0x00
    SECP256K1     = 0x01
    SECP256R1     = 0x02
    BIP32_ED25519 = 0x03


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


class MagicByte(IntEnum):
    """Class representing the magic byte."""

    BLOCK  = 0x01
    UNSAFE = 0x03


class StatusCode(IntEnum):
    """Class representing the status code."""

    OK = 0x9000


class TezosClient:
    """Class representing the tezos app client."""

    backend: BackendInterface

    def __init__(self, backend) -> None:
        self._client: BackendInterface = backend

    @contextmanager
    def authorize_baking(self, derivation_path: bytes) -> Generator[None, None, None]:
        """Send the AUTHORIZE_BAKING instruction."""
        with self._client.exchange_async(CLA,
                                         Ins.AUTHORIZE_BAKING,
                                         Index.FIRST,
                                         SigScheme.ED25519,
                                         derivation_path):
            yield

    @contextmanager
    def get_public_key_silent(self, derivation_path: bytes) -> Generator[None, None, None]:
        """Send the GET_PUBLIC_KEY instruction."""
        with self._client.exchange_async(CLA,
                                         Ins.GET_PUBLIC_KEY,
                                         Index.FIRST,
                                         SigScheme.ED25519,
                                         derivation_path):
            yield

    @contextmanager
    def get_public_key_prompt(self, derivation_path: bytes) -> Generator[None, None, None]:
        """Send the PROMPT_PUBLIC_KEY instruction."""
        with self._client.exchange_async(CLA,
                                         Ins.PROMPT_PUBLIC_KEY,
                                         Index.FIRST,
                                         SigScheme.ED25519,
                                         derivation_path):
            yield

    @contextmanager
    def reset_app_context(self, reset_level: int) -> Generator[None, None, None]:
        """Send the RESET instruction."""
        with self._client.exchange_async(CLA,
                                         Ins.RESET,
                                         Index.LAST,
                                         SigScheme.ED25519,
                                         reset_level.to_bytes(4, byteorder='big')):
            yield

    @contextmanager
    def setup_baking_address(self,
                             derivation_path: bytes,
                             chain: int,
                             main_hwm: int,
                             test_hwm: int) -> Generator[None, None, None]:
        """Send the SETUP instruction."""

        data: bytes = chain.to_bytes(4, byteorder='big') + main_hwm.to_bytes(
            4, byteorder='big') + test_hwm.to_bytes(4, byteorder='big') + derivation_path

        with self._client.exchange_async(CLA,
                                         Ins.SETUP,
                                         Index.FIRST,
                                         SigScheme.ED25519,
                                         data):
            yield

    @contextmanager
    def sign_message(self,
                     derivation_path: bytes,
                     operation_tag: OperationTag) -> Generator[None, None, None]:
        """Send the SIGN instruction."""

        self._client.exchange(CLA,
                              Ins.SIGN,
                              Index.FIRST,
                              SigScheme.BIP32_ED25519,
                              derivation_path)

        data: bytes = bytes.fromhex(
            CMD_PART1) + operation_tag.to_bytes(1, byteorder='big') + bytes.fromhex(CMD_PART2)

        with self._client.exchange_async(CLA,
                                         Ins.SIGN,
                                         Index.LAST,
                                         SigScheme.ED25519,
                                         MagicByte.UNSAFE.to_bytes(1, byteorder='big') + data):
            yield

    def get_async_response(self) -> Optional[RAPDU]:
        """Get an instruction response."""
        return self._client.last_async_response
