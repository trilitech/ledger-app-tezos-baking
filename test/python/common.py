"""Module gathering common values used in tests."""

from pathlib import Path
from utils.account import Account, SigScheme, BipPath

TESTS_ROOT_DIR = Path(__file__).parent

DEFAULT_SEED = " ".join(['zebra'] * 24)

DEFAULT_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.ED25519,
    "edsk2tUyhVvGj9B1S956ZzmaU4bC9J7t8xVBH52fkAoZL25MHEwacd"
)

EMPTY_PATH = BipPath.from_string("m")
