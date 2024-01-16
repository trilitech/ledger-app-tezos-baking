"""Module providing a tezos client."""

from typing import Tuple, Optional
from enum import IntEnum

from ragger.utils import RAPDU
from ragger.backend import BackendInterface
from ragger.error import ExceptionRAPDU
from utils.account import Account, SigScheme, BipPath
from utils.helper import BytesReader

CMD_PART1 = "17777d8de5596705f1cb35b0247b9605a7c93a7ed5c0caa454d4f4ff39eb411d"

CMD_PART2 = "00cf49f66b9ea137e11818f2a78b4b6fc9895b4e50830ae58003c35000c0843d0000eac6c762212c4110" \
    "f221ec8fcb05ce83db95845700"


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


MAX_APDU_SIZE: int = 235

class TezosClient:
    """Class representing the tezos app client."""

    backend: BackendInterface

    def __init__(self, backend) -> None:
        self._client: BackendInterface = backend

    def _exchange(self,
                  ins: Ins,
                  index: Index = Index.FIRST,
                  sig_scheme: Optional[SigScheme] = None,
                  payload: bytes = b'') -> bytes:

        assert len(payload) <= MAX_APDU_SIZE, "Apdu too large"

        # Set to a non-existent value to ensure that p2 is unused
        p2: int = sig_scheme if sig_scheme is not None else 0xff

        rapdu: RAPDU = self._client.exchange(Cla.DEFAULT,
                                             ins,
                                             p1=index,
                                             p2=p2,
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

    def authorize_baking(self, account: Account) -> bytes:
        """Send the AUTHORIZE_BAKING instruction."""
        return self._exchange(
            ins=Ins.AUTHORIZE_BAKING,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

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

    def get_public_key_silent(self, account: Account) -> bytes:
        """Send the GET_PUBLIC_KEY instruction."""
        return self._exchange(
            ins=Ins.GET_PUBLIC_KEY,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

    def get_public_key_prompt(self, account: Account) -> bytes:
        """Send the PROMPT_PUBLIC_KEY instruction."""
        return self._exchange(
            ins=Ins.PROMPT_PUBLIC_KEY,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

    def reset_app_context(self, reset_level: int) -> bytes:
        """Send the RESET instruction."""
        reset_level_raw = reset_level.to_bytes(4, byteorder='big')
        return self._exchange(
            ins=Ins.RESET,
            payload=reset_level_raw)

    def setup_baking_address(self,
                             account: Account,
                             chain: int,
                             main_hwm: int,
                             test_hwm: int) -> bytes:
        """Send the SETUP instruction."""

        data: bytes = b''
        data += chain.to_bytes(4, byteorder='big')
        data += main_hwm.to_bytes(4, byteorder='big')
        data += test_hwm.to_bytes(4, byteorder='big')
        data += bytes(account.path)

        return self._exchange(
            ins=Ins.SETUP,
            sig_scheme=account.sig_scheme,
            payload=data)

    def sign_message(self,
                     account: Account,
                     operation_tag: OperationTag) -> bytes:
        """Send the SIGN instruction."""

        data: bytes = b''

        data += MagicByte.UNSAFE.to_bytes(1, byteorder='big')
        data += bytes.fromhex(CMD_PART1)
        data += operation_tag.to_bytes(1, byteorder='big')
        data += bytes.fromhex(CMD_PART2)

        self._exchange(
            ins=Ins.SIGN,
            sig_scheme=account.sig_scheme,
            payload=bytes(account.path))

        return self._exchange(
            ins=Ins.SIGN,
            index=Index.LAST,
            payload=data)
