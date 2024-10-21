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

"""Module providing a tezos client."""

from typing import Tuple, Optional, Generator
from enum import IntEnum
from contextlib import contextmanager

from ragger.utils import RAPDU
from ragger.backend import BackendInterface
from ragger.error import ExceptionRAPDU
from pytezos.michelson import forge
from utils.account import Account, BipPath, PublicKey, Signature, SigScheme
from utils.helper import BytesReader
from utils.message import Message

class Version:
    """Class representing the version."""

    class AppKind(IntEnum):
        """Class representing the kind of app."""

        WALLET = 0x00
        BAKING = 0x01

    app_kind: AppKind
    major: int
    minor: int
    patch: int

    def __init__(self,
                 app_kind: AppKind,
                 major: int,
                 minor: int,
                 patch: int):
        self.app_kind = app_kind
        self.major = major
        self.minor = minor
        self.patch = patch

    def __repr__(self) -> str :
        return f"App {self.app_kind.name}: {self.major}.{self.minor}.{self.patch}"

    def __eq__(self, other: object):
        if not isinstance(other, Version):
            return NotImplemented
        return \
            self.app_kind == other.app_kind and \
            self.major == other.major and \
            self.minor == other.minor and \
            self.patch == other.patch

    @classmethod
    def from_bytes(cls, raw_version: bytes) -> 'Version':
        """Create a version from bytes."""

        reader = BytesReader(raw_version)
        app_kind = reader.read_int(1)
        major = reader.read_int(1)
        minor = reader.read_int(1)
        patch = reader.read_int(1)
        reader.assert_finished()

        return Version(Version.AppKind(app_kind), major, minor, patch)

class Hwm:
    """Class representing app high water mark."""

    highest_level: int
    highest_round: int

    def __init__(self,
                 highest_level: int,
                 highest_round: int,
            ):
        self.highest_level = highest_level
        self.highest_round = highest_round

    def __repr__(self) -> str :
        return f"(Level={self.highest_level}, Round={self.highest_round})"

    def __eq__(self, other: object):
        if not isinstance(other, Hwm):
            return NotImplemented
        return \
            self.highest_level == other.highest_level and \
            self.highest_round == other.highest_round

    def __bytes__(self) -> bytes :
        return forge.forge_int32(self.highest_level)

    @staticmethod
    def raw_length(migrated: bool) -> int:
        """Return the size of raw representation of hwm."""
        return 8 if migrated else 4

    @classmethod
    def from_bytes(cls, raw_hwm: bytes) -> 'Hwm':
        """Create a hwm from bytes."""

        reader = BytesReader(raw_hwm)
        highest_level = reader.read_int(4)
        highest_round = reader.read_int(4)
        reader.assert_finished()

        return Hwm(highest_level, highest_round)

class Cla(IntEnum):
    """Class representing APDU class."""

    DEFAULT = 0x80


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


class StatusCode(IntEnum):
    """Class representing the status code."""

    OK                        = 0x9000
    WRONG_PARAM               = 0x6b00
    WRONG_LENGTH              = 0x6c00
    INVALID_INS               = 0x6d00
    WRONG_LENGTH_FOR_INS      = 0x917e
    REJECT                    = 0x6985
    PARSE_ERROR               = 0x9405
    REFERENCED_DATA_NOT_FOUND = 0x6a88
    WRONG_VALUES              = 0x6a80
    SECURITY                  = 0x6982
    HID_REQUIRED              = 0x6983
    CLASS                     = 0x6e00
    MEMORY_ERROR              = 0x9200

    @contextmanager
    def expected(self) -> Generator[None, None, None]:
        """Fail if the right RAPDU code exception is not raise."""
        try:
            yield
            assert self == StatusCode.OK, \
                f"Expect fail with { self.name } but succeed"
        except ExceptionRAPDU as e:
            try:
                name = f"{StatusCode(e.status).name}"
            except ValueError:
                name = f"0x{e.status:x}"
            assert self == e.status, \
                f"Expect fail with { self.name } but fail with {name}"


MAX_APDU_SIZE: int = 235

class TezosClient:
    """Class representing the tezos app client."""

    backend: BackendInterface


    def __init__(self, backend) -> None:
        self.backend = backend

    def _exchange(self,
                  ins: Ins,
                  index: Index = Index.FIRST,
                  sig_scheme: SigScheme = SigScheme.DEFAULT,
                  payload: bytes = b'') -> bytes:

        assert len(payload) <= MAX_APDU_SIZE, "Apdu too large"

        rapdu: RAPDU = self.backend.exchange(Cla.DEFAULT,
                                             ins,
                                             p1=index,
                                             p2=sig_scheme,
                                             data=payload)

        if rapdu.status != StatusCode.OK:
            raise ExceptionRAPDU(rapdu.status, rapdu.data)

        return rapdu.data

    def version(self) -> Version:
        """Send the VERSION instruction."""
        return Version.from_bytes(self._exchange(ins=Ins.VERSION))

    def git(self) -> str:
        """Send the GIT instruction."""
        raw_commit = self._exchange(ins=Ins.GIT)
        assert raw_commit.endswith(b'\x00'), \
            f"Should end with by '\x00' but got {raw_commit.hex()}"
        return raw_commit[:-1].decode('utf-8')

    def authorize_baking(self, account: Optional[Account]) -> bytes:
        """Send the AUTHORIZE_BAKING instruction."""

        sig_scheme=SigScheme.DEFAULT # None will raise EXC_WRONG_PARAM
        payload=b''

        if account is not None:
            sig_scheme=account.sig_scheme
            payload=bytes(account.path)

        data = self._exchange(
            ins=Ins.AUTHORIZE_BAKING,
            sig_scheme=sig_scheme,
            payload=payload)

        length, data = data[0], data[1:]
        assert length == len(data), f"Wrong data size, {length} != {len(data)}"

        return data

    def deauthorize(self) -> None:
        """Send the DEAUTHORIZE instruction."""
        data = self._exchange(ins=Ins.DEAUTHORIZE)
        assert data == b'', f"No data expected but got {data.hex()}"

    def get_auth_key(self) -> BipPath:
        """Send the QUERY_AUTH_KEY instruction."""
        raw_path = self._exchange(ins=Ins.QUERY_AUTH_KEY)

        return BipPath.from_bytes(raw_path)

    def get_auth_key_with_curve(self) -> Tuple[SigScheme, BipPath]:
        """Send the QUERY_AUTH_KEY_WITH_CURVE instruction."""
        data = self._exchange(ins=Ins.QUERY_AUTH_KEY_WITH_CURVE)
        sig_scheme = SigScheme(data[0])
        return sig_scheme, BipPath.from_bytes(data[1:])

    def get_public_key_silent(self, account: Account) -> str:
        """Send the GET_PUBLIC_KEY instruction."""
        data = self._exchange(
            ins=Ins.GET_PUBLIC_KEY,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

        length, data = data[0], data[1:]
        assert length == len(data), f"Wrong data size, {length} != {len(data)}"

        return PublicKey.from_bytes(data, account.sig_scheme)

    def get_public_key_prompt(self, account: Account) -> str:
        """Send the PROMPT_PUBLIC_KEY instruction."""
        data = self._exchange(
            ins=Ins.PROMPT_PUBLIC_KEY,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

        length, data = data[0], data[1:]
        assert length == len(data), f"Wrong data size, {length} != {len(data)}"

        return PublicKey.from_bytes(data, account.sig_scheme)

    def reset_app_context(self, reset_level: int) -> None:
        """Send the RESET instruction."""
        reset_level_raw = reset_level.to_bytes(4, byteorder='big')
        data = self._exchange(
            ins=Ins.RESET,
            payload=reset_level_raw)
        assert data == b'', f"No data expected but got {data.hex()}"


    def setup_app_context(self,
                          account: Account,
                          main_chain_id: str,
                          main_hwm: Hwm,
                          test_hwm: Hwm) -> str:
        """Send the SETUP instruction."""

        data: bytes = b''
        data += forge.forge_base58(main_chain_id)
        data += bytes(main_hwm)
        data += bytes(test_hwm)
        data += bytes(account.path)

        data = self._exchange(
            ins=Ins.SETUP,
            sig_scheme=account.sig_scheme,
            payload=data)

        length, data = data[0], data[1:]
        assert length == len(data), f"Wrong data size, {length} != {len(data)}"

        return PublicKey.from_bytes(data, account.sig_scheme)

    def get_main_hwm(self) -> Hwm:
        """Send the QUERY_MAIN_HWM instruction."""
        return Hwm.from_bytes(self._exchange(ins=Ins.QUERY_MAIN_HWM))

    def get_all_hwm(self) -> Tuple[str, Hwm, Hwm]:
        """Send the QUERY_ALL_HWM instruction."""
        raw_data = self._exchange(ins=Ins.QUERY_ALL_HWM)

        reader = BytesReader(raw_data)
        migrated = len(raw_data) == 2 * Hwm.raw_length(migrated=True) + 4
        hwm_len = Hwm.raw_length(migrated=migrated)

        main_hwm = Hwm.from_bytes(reader.read_bytes(hwm_len))
        test_hwm = Hwm.from_bytes(reader.read_bytes(hwm_len))
        main_chain_id = forge.unforge_chain_id(reader.read_bytes(4))

        reader.assert_finished()

        return (main_chain_id, main_hwm, test_hwm)

    def sign_message(self,
                     account: Account,
                     message: Message) -> Signature:
        """Send the SIGN instruction."""

        self._exchange(
            ins=Ins.SIGN,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

        signature = self._exchange(
            ins=Ins.SIGN,
            index=Index.LAST,
            payload=bytes(message))

        return Signature.from_bytes(signature, account.sig_scheme)

    def sign_message_with_hash(self,
                     account: Account,
                     message: Message) -> Tuple[bytes, Signature]:
        """Send the SIGN_WITH_HASH instruction."""

        self._exchange(
            ins=Ins.SIGN_WITH_HASH,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

        data = self._exchange(
            ins=Ins.SIGN_WITH_HASH,
            index=Index.LAST,
            payload=bytes(message))

        return (
            data[:Message.HASH_SIZE],
            Signature.from_bytes(
                data[Message.HASH_SIZE:],
                account.sig_scheme
            )
        )

    def hmac(self,
             account: Account,
             message: bytes) -> bytes:
        """Send the HMAC instruction."""

        data: bytes = b''
        data += bytes(account.path)
        data += message

        return self._exchange(
            ins=Ins.HMAC,
            sig_scheme=account.sig_scheme,
            payload=data)
