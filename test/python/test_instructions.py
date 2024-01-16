"""Module gathering the baking app instruction tests."""

from pathlib import Path
import pytest
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from utils.client import TezosClient, Version
from utils.account import Account, SigScheme, BipPath
from utils.helper import (
    get_nano_review_instructions,
    get_stax_review_instructions,
    get_stax_address_instructions,
    get_public_key_flow_instructions,
    send_and_navigate,
    get_current_commit
)

TESTS_ROOT_DIR = Path(__file__).parent

DEFAULT_ACCOUNT = Account(
    "m/44'/1729'/0'/0'",
    SigScheme.ED25519,
    "edpkuXX2VdkdXzkN11oLCb8Aurdo1BTAtQiK8ZY9UPj2YMt3AHEpcY"
)

EMPTY_PATH = BipPath.from_string("m")


def test_version(client: TezosClient) -> None:
    """Test the VERSION instruction."""

    expected_version = Version(Version.AppKind.BAKING, 2, 4, 6)

    version = client.version()

    assert version == expected_version, \
        f"Expected {expected_version} but got {version}"

def test_git(client: TezosClient) -> None:
    """Test the GIT instruction."""

    expected_commit = get_current_commit()

    commit = client.git()

    assert commit == expected_commit, \
        f"Expected {expected_commit} but got {commit}"


def test_reset_hwm(
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the RESET instruction."""

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(3)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_stax_review_instructions(2)

    reset_level: int = 0

    send_and_navigate(
        send=lambda: client.reset_app_context(reset_level),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))


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

def test_get_public_key_baking(
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_address_instructions()

    send_and_navigate(
        send=lambda: client.get_public_key_prompt(DEFAULT_ACCOUNT),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))


def test_setup_baking_address(
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the SETUP instruction."""

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(8)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(7)
    else:
        instructions = get_stax_review_instructions(2)

    chain: int = 0
    main_hwm: int = 0
    test_hwm: int = 0

    send_and_navigate(
        send=lambda: client.setup_baking_address(
            DEFAULT_ACCOUNT,
            chain,
            main_hwm,
            test_hwm),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))


def test_get_public_key_silent(client: TezosClient) -> None:
    """Test the GET_PUBLIC_KEY instruction."""

    client.get_public_key_silent(DEFAULT_ACCOUNT)


def test_get_public_key_prompt(
        client: TezosClient,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_address_instructions()

    send_and_navigate(
        send=lambda: client.get_public_key_prompt(DEFAULT_ACCOUNT),
        navigate=lambda: navigator.navigate_and_compare(
            TESTS_ROOT_DIR,
            test_name,
            instructions))
