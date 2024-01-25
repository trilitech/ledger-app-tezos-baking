"""Module gathering common values used in tests."""

from pathlib import Path
from utils.account import Account, SigScheme, BipPath

TESTS_ROOT_DIR = Path(__file__).parent

DEFAULT_SEED = " ".join(['zebra'] * 24)

DEFAULT_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.ED25519,
    "edpkuXX2VdkdXzkN11oLCb8Aurdo1BTAtQiK8ZY9UPj2YMt3AHEpcY"
)

EMPTY_PATH = BipPath.from_string("m")
