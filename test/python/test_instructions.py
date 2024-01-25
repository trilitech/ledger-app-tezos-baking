"""Module gathering the baking app instruction tests."""

from pathlib import Path
import pytest
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from utils.client import TezosClient, Version, Hwm
from utils.account import Account
from utils.helper import (
    get_public_key_flow_instructions,
    get_setup_app_context_instructions,
    get_reset_app_context_instructions,
    send_and_navigate,
    get_current_commit
)
from utils.message import (
    Preattestation,
    Attestation,
    AttestationDal,
    Fitness,
    BlockHeader,
    Block
)
from common import (
    TESTS_ROOT_DIR,
    DEFAULT_ACCOUNT,
    EMPTY_PATH
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



@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_authorize_baking(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the AUTHORIZE_BAKING instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    public_key = send_and_navigate(
        send=lambda: client.authorize_baking(account),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions)
    )

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_deauthorize(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the DEAUTHORIZE instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    send_and_navigate(
        send=lambda: client.authorize_baking(account),
        navigate=lambda: navigator.navigate(instructions)
    )

    client.deauthorize()

    # get_auth_key_with_curve raise EXC_REFERENCED_DATA_NOT_FOUND

    path = client.get_auth_key()

    assert path == EMPTY_PATH, \
        f"Expected the empty path {EMPTY_PATH} but got {path}"

@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_auth_key(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the QUERY_AUTH_KEY instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    send_and_navigate(
        send=lambda: client.authorize_baking(account),
        navigate=lambda: navigator.navigate(instructions)
    )

    path = client.get_auth_key()

    assert path == account.path, \
        f"Expected {account.path} but got {path}"

@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_auth_key_with_curve(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the QUERY_AUTH_KEY_WITH_CURVE instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    send_and_navigate(
        send=lambda: client.authorize_baking(account),
        navigate=lambda: navigator.navigate(instructions)
    )

    sig_scheme, path = client.get_auth_key_with_curve()

    assert path == account.path, \
        f"Expected {account.path} but got {path}"

    assert sig_scheme == account.sig_scheme, \
        f"Expected {account.sig_scheme.name} but got {sig_scheme.name}"

@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_public_key_baking(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the AUTHORIZE_BAKING instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    send_and_navigate(
        send=lambda: client.authorize_baking(account),
        navigate=lambda: navigator.navigate(instructions)
    )

    public_key = send_and_navigate(
        send=lambda: client.authorize_baking(None),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions)
    )

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_public_key_silent(account: Account, client: TezosClient) -> None:
    """Test the GET_PUBLIC_KEY instruction."""

    public_key = client.get_public_key_silent(account)

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_public_key_prompt(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    instructions = get_public_key_flow_instructions(firmware)

    public_key = send_and_navigate(
        send=lambda: client.get_public_key_prompt(account),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))

    account.check_public_key(public_key)


def test_reset_app_context(
        client: TezosClient,
        firmware,
        navigator,
        test_name) -> None:
    """Test the RESET instruction."""

    instructions = get_reset_app_context_instructions(firmware)

    reset_level: int = 0

    send_and_navigate(
        send=lambda: client.reset_app_context(reset_level),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions)
    )


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_setup_app_context(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the SETUP instruction."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    public_key = send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))

    account.check_public_key(public_key)


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_main_hwm(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the QUERY_MAIN_HWM instruction."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    received_main_hwm = client.get_main_hwm()

    assert received_main_hwm == main_hwm, \
        f"Expected main hmw {main_hwm} but got {received_main_hwm}"


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
def test_get_all_hwm(
        account: Account,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the QUERY_ALL_HWM instruction."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    received_main_chain_id, received_main_hwm, received_test_hwm = client.get_all_hwm()

    assert received_main_chain_id == main_chain_id, \
        f"Expected main chain id {main_chain_id} but got {received_main_chain_id}"

    assert received_main_hwm == main_hwm, \
        f"Expected main hmw {main_hwm} but got {received_main_hwm}"

    assert received_test_hwm == test_hwm, \
        f"Expected test hmw {test_hwm} but got {received_test_hwm}"


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_preattestation(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on preattestation."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    preattestation = Preattestation(
        chain_id=main_chain_id,
        branch="0000000000000000000000000000000000000000000000000000000000000000",
        slot=0,
        op_level=0,
        op_round=0,
        block_payload_hash="0000000000000000000000000000000000000000000000000000000000000000"
    )

    if with_hash:
        signature = client.sign_message(account, preattestation)
        account.check_signature(signature, bytes(preattestation))
    else:
        preattestation_hash, signature = client.sign_message_with_hash(account, preattestation)
        assert preattestation_hash == preattestation.hash, \
            f"Expected hash {preattestation.hash.hex()} but got {preattestation_hash.hex()}"
        account.check_signature(signature, bytes(preattestation))


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    attestation = Attestation(
        chain_id=main_chain_id,
        branch="0000000000000000000000000000000000000000000000000000000000000000",
        slot=0,
        op_level=0,
        op_round=0,
        block_payload_hash="0000000000000000000000000000000000000000000000000000000000000000"
    )

    if with_hash:
        signature = client.sign_message(account, attestation)
        account.check_signature(signature, bytes(attestation))
    else:
        attestation_hash, signature = client.sign_message_with_hash(account, attestation)
        assert attestation_hash == attestation.hash, \
            f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
        account.check_signature(signature, bytes(attestation))


@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation_dal(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    attestation = AttestationDal(
        chain_id=main_chain_id,
        branch="0000000000000000000000000000000000000000000000000000000000000000",
        slot=0,
        op_level=0,
        op_round=0,
        block_payload_hash="0000000000000000000000000000000000000000000000000000000000000000",
        dal_message="0000000000000000000000000000000000000000000000000000000000000000"
    )

    if with_hash:
        signature = client.sign_message(account, attestation)
        account.check_signature(signature, bytes(attestation))
    else:
        attestation_hash, signature = client.sign_message_with_hash(account, attestation)
        assert attestation_hash == attestation.hash, \
            f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
        account.check_signature(signature, bytes(attestation))




@pytest.mark.parametrize("account", [DEFAULT_ACCOUNT])
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_block(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on block."""

    main_chain_id: int = 0
    main_hwm = Hwm(0)
    test_hwm = Hwm(0)

    instructions = get_setup_app_context_instructions(firmware)

    send_and_navigate(
        send=lambda: client.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate(instructions))

    fitness = Fitness(
        level=0,
        locked_round=0,
        predecessor_round=0,
        current_round=0
    )

    block_header = BlockHeader(
        level=0,
        proto_level=0,
        predecessor="0000000000000000000000000000000000000000000000000000000000000000",
        timestamp=0,
        validation_pass=0,
        operations_hash="0000000000000000000000000000000000000000000000000000000000000000",
        fitness=fitness,
        context="0000000000000000000000000000000000000000000000000000000000000000"
    )

    block = Block(
        chain_id=main_chain_id,
        header=block_header,
        data=b''
    )

    if with_hash:
        signature = client.sign_message(account, block)
        account.check_signature(signature, bytes(block))
    else:
        block_hash, signature = client.sign_message_with_hash(account, block)
        assert block_hash == block.hash, \
            f"Expected hash {block.hash.hex()} but got {block_hash.hex()}"
        account.check_signature(signature, bytes(block))

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
