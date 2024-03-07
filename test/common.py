"""Module gathering common values used in tests."""

from pathlib import Path
from utils.account import Account, SigScheme, BipPath

TESTS_ROOT_DIR = Path(__file__).parent

DEFAULT_SEED = " ".join(['zebra'] * 24)

TZ1_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.ED25519,
    "edsk2tUyhVvGj9B1S956ZzmaU4bC9J7t8xVBH52fkAoZL25MHEwacd"
)

TZ2_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.SECP256K1,
    "spsk2Pfx9chqXVbz2tW7ze4gGU4RfaiK3nSva77bp69zHhFho2zTze"
)

TZ3_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.SECP256R1,
    "p2sk2zPCmKo6zTSjPbDHnLiHtPAqVRFrExN3oTvKGbu3C99Jyeyura"
)

BIP32_TZ1_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.BIP32_ED25519,
    "edsk2oM2vowLX6m5wtTnkBNK3PCPL2rTow1U4MQmxZiQtJQc65KP5i"
)

LONG_TZ1_ACCOUNT = Account(
    "m/9'/12'/13'/8'/78'",
    SigScheme.ED25519,
    "edsk3eZBgFAf1VtdibfxoCcihxXje9S3th7jdEgVA2kHG82EKYNKNm"
)

DEFAULT_ACCOUNT = TZ1_ACCOUNT

DEFAULT_ACCOUNT_2 = TZ2_ACCOUNT

TZ1_ACCOUNTS = [
    TZ1_ACCOUNT,
    LONG_TZ1_ACCOUNT,
    BIP32_TZ1_ACCOUNT,
]

TZ2_ACCOUNTS = [
    TZ2_ACCOUNT,
]

TZ3_ACCOUNTS = [
    TZ3_ACCOUNT,
]

ACCOUNTS = TZ1_ACCOUNTS + TZ2_ACCOUNTS + TZ3_ACCOUNTS

EMPTY_PATH = BipPath.from_string("m")
