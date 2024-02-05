"""Module gathering the baking app instruction tests."""

from pathlib import Path
from typing import Callable, Tuple
import pytest
from ragger.firmware import Firmware
from ragger.navigator import NavInsID
from utils.client import TezosClient, Version, Hwm, StatusCode
from utils.account import Account
from utils.helper import get_current_commit
from utils.message import (
    Message,
    Preattestation,
    Attestation,
    AttestationDal,
    Fitness,
    BlockHeader,
    Block,
    DEFAULT_CHAIN_ID
)
from utils.navigator import TezosNavigator
from common import (
    DEFAULT_ACCOUNT,
    DEFAULT_ACCOUNT_2,
    ACCOUNTS,
)


def test_review_home(firmware: Firmware, tezos_navigator: TezosNavigator) -> None:
    """Test the display of the home/info pages."""

    instructions = [NavInsID.RIGHT_CLICK] * 5 if firmware.is_nano else \
        [
            NavInsID.USE_CASE_HOME_SETTINGS,
            NavInsID.USE_CASE_SETTINGS_NEXT
        ]

    tezos_navigator.navigate_and_compare(
        snap_path=Path(""),
        instructions=instructions,
        screen_change_before_first_instruction=False
    )


def test_version(client: TezosClient) -> None:
    """Test the VERSION instruction."""

    expected_version = Version(Version.AppKind.BAKING, 2, 4, 7)

    version = client.version()

    assert version == expected_version, \
        f"Expected {expected_version} but got {version}"

def test_git(client: TezosClient) -> None:
    """Test the GIT instruction."""

    expected_commit = get_current_commit()

    commit = client.git()

    assert commit == expected_commit, \
        f"Expected {expected_commit} but got {commit}"


@pytest.mark.parametrize("account", ACCOUNTS)
def test_authorize_baking(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the AUTHORIZE_BAKING instruction."""
    snap_path = Path(f"{account}")

    public_key = tezos_navigator.authorize_baking(account, snap_path=snap_path)

    account.check_public_key(public_key)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0),
        test_hwm=Hwm(0),
        snap_path=snap_path
    )


def test_deauthorize(client: TezosClient, tezos_navigator: TezosNavigator) -> None:
    """Test the DEAUTHORIZE instruction."""

    account = DEFAULT_ACCOUNT

    tezos_navigator.authorize_baking(account)

    client.deauthorize()

    tezos_navigator.check_app_context(
        None,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0),
        test_hwm=Hwm(0)
    )

@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_auth_key(
        account: Account,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the QUERY_AUTH_KEY instruction."""

    tezos_navigator.authorize_baking(account)

    path = client.get_auth_key()

    assert path == account.path, \
        f"Expected {account.path} but got {path}"

@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_auth_key_with_curve(
        account: Account,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the QUERY_AUTH_KEY_WITH_CURVE instruction."""

    tezos_navigator.authorize_baking(account)

    sig_scheme, path = client.get_auth_key_with_curve()

    assert path == account.path, \
        f"Expected {account.path} but got {path}"

    assert sig_scheme == account.sig_scheme, \
        f"Expected {account.sig_scheme.name} but got {sig_scheme.name}"

@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_public_key_baking(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the AUTHORIZE_BAKING instruction."""

    tezos_navigator.authorize_baking(account)

    public_key = tezos_navigator.authorize_baking(None, snap_path=Path(f"{account}"))

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_public_key_silent(account: Account, client: TezosClient) -> None:
    """Test the GET_PUBLIC_KEY instruction."""

    public_key = client.get_public_key_silent(account)

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_public_key_prompt(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    public_key = tezos_navigator.get_public_key_prompt(account, snap_path=Path(f"{account}"))

    account.check_public_key(public_key)


def test_reset_app_context(tezos_navigator: TezosNavigator) -> None:
    """Test the RESET instruction."""

    reset_level: int = 1

    tezos_navigator.reset_app_context(reset_level, snap_path=Path(""))

    tezos_navigator.check_app_context(
        None,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(reset_level),
        test_hwm=Hwm(reset_level)
    )


@pytest.mark.parametrize("account", ACCOUNTS)
def test_setup_app_context(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the SETUP instruction."""
    snap_path = Path(f"{account}")

    main_chain_id = "NetXH12AexHqTQa"
    main_hwm = Hwm(1)
    test_hwm = Hwm(2)

    public_key = tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm,
        snap_path=snap_path
    )

    account.check_public_key(public_key)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=main_hwm,
        test_hwm=test_hwm,
        snap_path=snap_path
    )


@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_main_hwm(
        account: Account,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the QUERY_MAIN_HWM instruction."""

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    received_main_hwm = client.get_main_hwm()

    assert received_main_hwm == main_hwm, \
        f"Expected main hmw {main_hwm} but got {received_main_hwm}"


@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_all_hwm(
        account: Account,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the QUERY_ALL_HWM instruction."""

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    received_main_chain_id, received_main_hwm, received_test_hwm = client.get_all_hwm()

    assert received_main_chain_id == main_chain_id, \
        f"Expected main chain id {main_chain_id} but got {received_main_chain_id}"

    assert received_main_hwm == main_hwm, \
        f"Expected main hmw {main_hwm} but got {received_main_hwm}"

    assert received_test_hwm == test_hwm, \
        f"Expected test hmw {test_hwm} but got {received_test_hwm}"


def build_preattestation(op_level, op_round, chain_id):
    """Build a preattestation."""
    return Preattestation(
        op_level=op_level,
        op_round=op_round
    ).forge(chain_id=chain_id)

def build_attestation(op_level, op_round, chain_id):
    """Build a attestation."""
    return Attestation(
        op_level=op_level,
        op_round=op_round
    ).forge(chain_id=chain_id)

def build_attestation_dal(op_level, op_round, chain_id):
    """Build a attestation_dal."""
    return AttestationDal(
        op_level=op_level,
        op_round=op_round
    ).forge(chain_id=chain_id)

def build_block(level, current_round, chain_id):
    """Build a block."""
    return Block(
        header=BlockHeader(
            level=level,
            fitness=Fitness(current_round=current_round)
        )
    ).forge(chain_id=chain_id)


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_preattestation(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on preattestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    preattestation = build_preattestation(
        op_level=1,
        op_round=2,
        chain_id=main_chain_id
    )

    if with_hash:
        signature = client.sign_message(account, preattestation)
        account.check_signature(signature, bytes(preattestation))
    else:
        preattestation_hash, signature = client.sign_message_with_hash(account, preattestation)
        assert preattestation_hash == preattestation.hash, \
            f"Expected hash {preattestation.hash.hex()} but got {preattestation_hash.hex()}"
        account.check_signature(signature, bytes(preattestation))

    tezos_navigator.assert_screen(
        name="black_screen",
        snap_path=snap_path / "app_context"
    )

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0),
        snap_path=snap_path
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    attestation = build_attestation(
        op_level=1,
        op_round=2,
        chain_id=main_chain_id
    )

    if with_hash:
        signature = client.sign_message(account, attestation)
        account.check_signature(signature, bytes(attestation))
    else:
        attestation_hash, signature = client.sign_message_with_hash(account, attestation)
        assert attestation_hash == attestation.hash, \
            f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
        account.check_signature(signature, bytes(attestation))

    tezos_navigator.assert_screen(
        name="black_screen",
        snap_path=snap_path / "app_context"
    )

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0),
        snap_path=snap_path
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation_dal(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    attestation = build_attestation_dal(
        op_level=1,
        op_round=2,
        chain_id=main_chain_id
    )

    if with_hash:
        signature = client.sign_message(account, attestation)
        account.check_signature(signature, bytes(attestation))
    else:
        attestation_hash, signature = client.sign_message_with_hash(account, attestation)
        assert attestation_hash == attestation.hash, \
            f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
        account.check_signature(signature, bytes(attestation))

    tezos_navigator.assert_screen(
        name="black_screen",
        snap_path=snap_path / "app_context"
    )

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0),
        snap_path=snap_path
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_block(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on block."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    block = build_block(
        level=1,
        current_round=2,
        chain_id=main_chain_id
    )

    if with_hash:
        signature = client.sign_message(account, block)
        account.check_signature(signature, bytes(block))
    else:
        block_hash, signature = client.sign_message_with_hash(account, block)
        assert block_hash == block.hash, \
            f"Expected hash {block.hash.hex()} but got {block_hash.hex()}"
        account.check_signature(signature, bytes(block))

    tezos_navigator.assert_screen(
        name="black_screen",
        snap_path=snap_path / "app_context"
    )

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0),
        snap_path=snap_path
    )


def test_sign_block_at_reset_level(client: TezosClient, tezos_navigator: TezosNavigator) -> None:
    """Test that signing block at reset level fails."""

    account = DEFAULT_ACCOUNT

    reset_level: int = 1

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(reset_level)
    test_hwm = Hwm(0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )

    block = build_block(
        level=reset_level,
        current_round=0,
        chain_id=main_chain_id
    )

    with StatusCode.WRONG_VALUES.expected():
        client.sign_message(account, block)


PARAMETERS = [
    (build_attestation,    (0, 0), build_preattestation,  (0, 1), True ),
    (build_block,          (0, 1), build_attestation_dal, (1, 0), True ),
    (build_block,          (0, 1), build_attestation_dal, (0, 0), False),
    (build_attestation,    (1, 0), build_preattestation,  (0, 1), False),
] + [
    (build_preattestation,  (0, 0), build_preattestation,  (0, 0), False),
    (build_preattestation,  (0, 0), build_block,           (0, 0), False),
    (build_preattestation,  (0, 0), build_attestation,     (0, 0), True ),
    (build_preattestation,  (0, 0), build_attestation_dal, (0, 0), True ),
    (build_attestation,     (0, 0), build_preattestation,  (0, 0), False),
    (build_attestation,     (0, 0), build_attestation,     (0, 0), False),
    (build_attestation,     (0, 0), build_attestation_dal, (0, 0), False),
    (build_attestation,     (0, 0), build_block,           (0, 0), False),
    (build_attestation_dal, (0, 0), build_preattestation,  (0, 0), False),
    (build_attestation_dal, (0, 0), build_attestation,     (0, 0), False),
    (build_attestation_dal, (0, 0), build_attestation_dal, (0, 0), False),
    (build_attestation_dal, (0, 0), build_block,           (0, 0), False),
    (build_block,           (1, 1), build_preattestation,  (1, 1), True ),
    (build_block,           (1, 1), build_attestation,     (1, 1), True ),
    (build_block,           (1, 1), build_attestation_dal, (1, 1), True ),
    (build_block,           (1, 1), build_block,           (1, 1), False),
]

@pytest.mark.parametrize(
    "message_builder_1, level_round_1, " \
    "message_builder_2, level_round_2, " \
    "success",
    PARAMETERS)
def test_sign_level_authorized(
        message_builder_1: Callable[[int, int, str], Message],
        level_round_1: Tuple[int, int],
        message_builder_2: Callable[[int, int, str], Message],
        level_round_2: Tuple[int, int],
        success: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test whether the level/round constraints behave as expected."""

    account: Account = DEFAULT_ACCOUNT

    main_chain_id = DEFAULT_CHAIN_ID
    main_level = 1

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm=Hwm(main_level),
        test_hwm=Hwm(0)
    )

    level_1, round_1 = level_round_1
    message_1 = message_builder_1(
        level_1 + main_level,
        round_1,
        main_chain_id
    )
    level_2, round_2 = level_round_2
    message_2 = message_builder_2(
        level_2 + main_level,
        round_2,
        main_chain_id
    )

    client.sign_message(account, message_1)

    if success:
        client.sign_message(account, message_2)
    else:
        with StatusCode.WRONG_VALUES.expected():
            client.sign_message(account, message_2)

def test_sign_not_authorized_key(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing with a key different from the authorized key is not authorized.."""

    account_1 = DEFAULT_ACCOUNT
    account_2 = DEFAULT_ACCOUNT_2

    main_chain_id = DEFAULT_CHAIN_ID

    tezos_navigator.setup_app_context(
        account_1,
        main_chain_id,
        main_hwm=Hwm(0),
        test_hwm=Hwm(0)
    )

    attestation = build_attestation(0, 0, main_chain_id)

    with StatusCode.SECURITY.expected():
        client.sign_message(account_2, attestation)


# Data generated by the old application itself
HMAC_TEST_SET = [
    (DEFAULT_ACCOUNT,
     bytes.fromhex("00"),
     bytes.fromhex("19248aa5277e4b53e817dc75d2eee7bac7e5e5c288293b978ca7769673cb16f1")),
    (DEFAULT_ACCOUNT,
     bytes.fromhex("000000000000000000000000000000000000000000000000"),
     bytes.fromhex("7993eae12110a43c7263ea3a407e21d224a66a36f61bfbcc465d13cdf56a70fe")),
    (DEFAULT_ACCOUNT,
     bytes.fromhex("0123456789abcdef"),
     bytes.fromhex("dde436940ab1602029a49dc77e6263b633fa3567c4d8479820d4f77072369ac1")),
    (DEFAULT_ACCOUNT,
     bytes.fromhex("0123456789abcdef0123456789abcdef0123456789abcdef"),
     bytes.fromhex("40f05a3f6f53f1e8805080f4c3631c6e67df5de20f5b697362521c2d7bc148b7"))
]

@pytest.mark.parametrize("account, message, expected_hmac", HMAC_TEST_SET)
def test_hmac(
        account: Account,
        message: bytes,
        expected_hmac: bytes,
        client: TezosClient) -> None:
    """Test the HMAC instruction."""

    data = client.hmac(account, message)
    assert data == expected_hmac, f"Expected HMAC {expected_hmac.hex()} but got {data.hex()}"
