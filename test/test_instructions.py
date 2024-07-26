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

"""Module gathering the baking app instruction tests."""

from pathlib import Path
from typing import Callable, Optional, Tuple

import hashlib
import hmac
import time

import pytest
from pytezos import pytezos

from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from utils.client import TezosClient, Version, Hwm, StatusCode
from utils.account import Account
from utils.helper import get_current_commit
from utils.message import (
    Message,
    UnsafeOp,
    Delegation,
    Reveal,
    Preattestation,
    Attestation,
    AttestationDal,
    Fitness,
    BlockHeader,
    Block,
    DEFAULT_CHAIN_ID
)
from utils.navigator import (
    TezosNavigator,
    NanoFixedScreen,
    TouchFixedScreen,
    send_and_navigate
)
from common import (
    DEFAULT_ACCOUNT,
    DEFAULT_ACCOUNT_2,
    TZ1_ACCOUNTS,
    ACCOUNTS,
    ZEBRA_ACCOUNTS,
)

@pytest.mark.parametrize("account", [None, *ACCOUNTS])
def test_review_home(account: Optional[Account],
                     backend: BackendInterface,
                     firmware: Firmware,
                     tezos_navigator: TezosNavigator) -> None:
    """Test the display of the home/info pages."""
    snap_path = \
        Path("") if account is None else \
        Path(f"{account}")

    if account is not None:
        main_chain_id = "NetXH12AexHqTQa" # Chain = 1
        main_hwm = Hwm(1,0)
        test_hwm = Hwm(2,0)

        tezos_navigator.setup_app_context(
            account,
            main_chain_id,
            main_hwm,
            test_hwm
        )

    if firmware.is_nano:
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_VERSION)
        tezos_navigator.right()
        tezos_navigator.assert_screen("chain_id", snap_path)
        tezos_navigator.right()
        if account is not None and firmware.device == "nanos":
            for i in range(1, account.nanos_screens + 1):
                tezos_navigator.assert_screen("public_key_hash_" + str(i), snap_path)
                tezos_navigator.right()
        else:
            tezos_navigator.assert_screen("public_key_hash", snap_path)
            tezos_navigator.right()
        tezos_navigator.assert_screen("high_watermark", snap_path)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_SETTINGS)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_QUIT)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_QUIT)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_SETTINGS)
        tezos_navigator.left()
        tezos_navigator.assert_screen("high_watermark", snap_path)
        tezos_navigator.left()
        if account is not None and firmware.device == "nanos":
            for i in reversed(range(1, account.nanos_screens + 1)):
                tezos_navigator.assert_screen("public_key_hash_" + str(i), snap_path)
                tezos_navigator.left()
        else:
            tezos_navigator.assert_screen("public_key_hash", snap_path)
            tezos_navigator.left()
        tezos_navigator.assert_screen("chain_id", snap_path)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_VERSION)
        tezos_navigator.right()
        tezos_navigator.assert_screen("chain_id", snap_path)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_VERSION)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_QUIT)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_SETTINGS)
        tezos_navigator.left()
        tezos_navigator.assert_screen("high_watermark", snap_path)
        tezos_navigator.right()
        # Check settings menu
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_SETTINGS)
        tezos_navigator.press_both_buttons()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.press_both_buttons()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_HMW_DISABLED)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_BACK)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_HMW_DISABLED)
        tezos_navigator.press_both_buttons()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.SETTINGS_BACK)
        tezos_navigator.press_both_buttons()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
        tezos_navigator.left()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_QUIT)
        tezos_navigator.right()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
        tezos_navigator.left()
    else:
        backend.wait_for_home_screen()
        tezos_navigator.home.settings()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.settings.next()
        backend.wait_for_screen_change()
        if tezos_navigator.firmware == Firmware.STAX:
            # chain_id + pkh + hwm
            tezos_navigator.assert_screen("app_context", snap_path)
        elif tezos_navigator.firmware == Firmware.FLEX:
            # chain_id + pkh
            tezos_navigator.assert_screen("app_context_1", snap_path)
            tezos_navigator.settings.next()
            backend.wait_for_screen_change()
            # hwm + version
            tezos_navigator.assert_screen("app_context_2", snap_path)
        tezos_navigator.settings.next()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_DESCRIPTION)
        tezos_navigator.settings.previous()
        backend.wait_for_screen_change()
        if tezos_navigator.firmware == Firmware.STAX:
            # chain_id + pkh + hwm
            tezos_navigator.assert_screen("app_context", snap_path)
        elif tezos_navigator.firmware == Firmware.FLEX:
            # hwm + version
            tezos_navigator.assert_screen("app_context_2", snap_path)
            tezos_navigator.settings.previous()
            backend.wait_for_screen_change()
            # chain_id + pkh
            tezos_navigator.assert_screen("app_context_1", snap_path)
        tezos_navigator.settings.previous()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.settings.exit()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.HOME)
        tezos_navigator.home.settings()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.settings.toggle_hwm_status()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_DISABLED)
        tezos_navigator.settings.next()
        backend.wait_for_screen_change()
        if tezos_navigator.firmware == Firmware.STAX:
            # chain_id + pkh + hwm
            tezos_navigator.assert_screen("app_context", snap_path)
        elif tezos_navigator.firmware == Firmware.FLEX:
            # chain_id + pkh
            tezos_navigator.assert_screen("app_context_1", snap_path)
            tezos_navigator.settings.next()
            backend.wait_for_screen_change()
            # hwm + version
            tezos_navigator.assert_screen("app_context_2", snap_path)
        tezos_navigator.settings.next()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_DESCRIPTION)
        tezos_navigator.settings.previous()
        backend.wait_for_screen_change()
        if tezos_navigator.firmware == Firmware.STAX:
            # chain_id + pkh + hwm
            tezos_navigator.assert_screen("app_context", snap_path)
        elif tezos_navigator.firmware == Firmware.FLEX:
            # hwm + version
            tezos_navigator.assert_screen("app_context_2", snap_path)
            tezos_navigator.settings.previous()
            backend.wait_for_screen_change()
            # chain_id + pkh
            tezos_navigator.assert_screen("app_context_1", snap_path)
        tezos_navigator.settings.previous()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_DISABLED)
        tezos_navigator.settings.toggle_hwm_status()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.SETTINGS_HMW_ENABLED)
        tezos_navigator.settings.exit()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(TouchFixedScreen.HOME)


def test_low_cost_screensaver(firmware: Firmware,
                              backend: BackendInterface,
                              tezos_navigator: TezosNavigator) -> None:
    """Test if the low-cost screensaver work as intended."""

    if firmware.name != "nanos":
        pytest.skip("Only on nanos devices")

    all_click = [
        backend.both_click,
        backend.left_click,
        backend.right_click,
    ]
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)
    for click in all_click:
        backend.both_click()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_BLACK)
        click()
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

def test_automatic_low_cost_screensaver(firmware: Firmware,
                                        backend: BackendInterface,
                                        client: TezosClient,
                                        tezos_navigator: TezosNavigator) -> None:
    """Test the low-cost screensaver activate at sign."""

    if firmware.name != "nanos":
        pytest.skip("Only on nanos devices")

    account = DEFAULT_ACCOUNT

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        Hwm(0, 0),
        Hwm(0, 0)
    )

    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver activate only after signing
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    attestation = build_attestation(
        op_level=1,
        op_round=0,
        chain_id=DEFAULT_CHAIN_ID
    )

    client.sign_message(account, attestation)

    time.sleep(5)

    # Low-cost screensaver activate after 20s after signing
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver has been activated
    backend.wait_for_screen_change()
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_BLACK)

    backend.both_click()
    backend.wait_for_screen_change()
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver deactivate after button push
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

def test_automatic_low_cost_screensaver_cancelled_by_display(
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test that low-cost screensaver is cancelled by display"""

    if firmware.name != "nanos":
        pytest.skip("Only on nanos devices")

    account = DEFAULT_ACCOUNT

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        Hwm(0, 0),
        Hwm(0, 0)
    )

    attestation = build_attestation(
        op_level=1,
        op_round=0,
        chain_id=DEFAULT_CHAIN_ID
    )

    client.sign_message(account, attestation)

    time.sleep(5)

    # Low-cost screensaver activate after 20s after signing
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    def delayed_authorize_navigate(**kwargs):
        time.sleep(30)

        # Low-cost screensaver deactivate after something is displayed.
        backend.wait_for_screen_change()
        tezos_navigator.assert_screen("first_authorize_screen")

        tezos_navigator.accept_key_navigate(
            screen_change_before_first_instruction=False,
            **kwargs
        )

    tezos_navigator.authorize_baking(
        account,
        navigate=delayed_authorize_navigate
    )

def test_automatic_low_cost_screensaver_exited_by_display(
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test that low-cost screensaver is exited by display"""

    if firmware.name != "nanos":
        pytest.skip("Only on nanos devices")

    account = DEFAULT_ACCOUNT

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        Hwm(0, 0),
        Hwm(0, 0)
    )

    attestation = build_attestation(
        op_level=1,
        op_round=0,
        chain_id=DEFAULT_CHAIN_ID
    )

    client.sign_message(account, attestation)

    time.sleep(5)

    # Low-cost screensaver activate after 20s after signing
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver has been activated
    backend.wait_for_screen_change()
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_BLACK)

    # Exit the low-cost screensaver by display
    tezos_navigator.authorize_baking(account, snap_path=Path("authorize"))

    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver deactivate after display
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    attestation = build_attestation(
        op_level=2,
        op_round=0,
        chain_id=DEFAULT_CHAIN_ID
    )

    client.sign_message(account, attestation)

    time.sleep(5)

    # Low-cost screensaver activate after 20s after signing
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_WELCOME)

    time.sleep(30)

    # Low-cost screensaver has been activated
    backend.wait_for_screen_change()
    tezos_navigator.assert_screen(NanoFixedScreen.HOME_BLACK)


def test_version(client: TezosClient) -> None:
    """Test the VERSION instruction."""

    expected_version = Version(Version.AppKind.BAKING, 2, 5, 0)

    version = client.version()

    assert version == expected_version, \
        f"Expected {expected_version} but got {version}"

def test_git(client: TezosClient) -> None:
    """Test the GIT instruction."""

    expected_commit = get_current_commit()

    commit = client.git()

    assert commit == expected_commit, \
        f"Expected {expected_commit} but got {commit}"


def test_ledger_screensaver(firmware: Firmware,
                            client: TezosClient,
                            tezos_navigator: TezosNavigator,
                            backend_name) -> None:
    # Make sure that ledger device being tested has screensaver time set to 1 minute and PIN lock is disabled.
    account = DEFAULT_ACCOUNT
    if backend_name == "speculos":
        assert True
        return

    time.sleep(70)

    res = input("Has the Ledger screensaver been activated?")
    assert (res.find("y") != -1), "Ledger screensaver should have activated"

    lvl = 0
    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(lvl, 0)
    test_hwm = Hwm(0, 0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )
    for _ in range(120):
        lvl += 1
        attestation = build_attestation(
            op_level=lvl,
            op_round=0,
            chain_id=main_chain_id
        )
        client.sign_message(account, attestation)
        time.sleep(1)

    res = input("Has the Ledger screensaver been activated?")
    if firmware.device == "nanos":
        assert (res.find("y") == -1), "Ledger screensaver should not have been activated"
    else:
        assert (res.find("y") != -1), "Ledger screensaver should have activated"


@pytest.mark.parametrize("account", ZEBRA_ACCOUNTS)
def test_benchmark_attestation_time(account: Account, client: TezosClient, tezos_navigator: TezosNavigator, backend_name) -> None:
    # check if backend is speculos, then return .
    if backend_name == "speculos":
        assert True
        return

    lvl = 0
    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(lvl, 0)
    test_hwm = Hwm(0, 0)

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm,
        test_hwm
    )
    st = time.time()
    # Run test for 100 times.
    for _ in range(100):
        lvl += 1
        attestation = build_attestation(
            op_level=lvl,
            op_round=0,
            chain_id=main_chain_id
        )
        client.sign_message(account, attestation)
    end= time.time()
    with open("Avg_time_for_100_attestations.txt",'a') as f:
        f.write("\nTime elapsed for derivation type : " + str(account) + " is : " + str(end-st) + "\n")


@pytest.mark.parametrize("account", ACCOUNTS)
def test_authorize_baking(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the AUTHORIZE_BAKING instruction."""
    snap_path = Path(f"{account}")

    public_key = tezos_navigator.authorize_baking(account, snap_path=snap_path)

    account.check_public_key(public_key)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )


def test_deauthorize(firmware: Firmware,
                     backend: BackendInterface,
                     client: TezosClient,
                     tezos_navigator: TezosNavigator) -> None:
    """Test the DEAUTHORIZE instruction."""

    account = DEFAULT_ACCOUNT

    tezos_navigator.authorize_baking(account)

    with tezos_navigator.goto_home_public_key():
        tezos_navigator.assert_screen("authorized_key_before_authorize")

        client.deauthorize()

        if firmware.is_nano:
            # No update for Stax or flex
            backend.wait_for_screen_change()
        if firmware.device == "nanos":
            # Wait blink
            time.sleep(0.5)
        tezos_navigator.assert_screen("authorized_key_after_authorize")

    tezos_navigator.check_app_context(
        None,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
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
        main_hwm=Hwm(reset_level, 0),
        test_hwm=Hwm(reset_level, 0)
    )


@pytest.mark.parametrize("account", ACCOUNTS)
def test_setup_app_context(account: Account, tezos_navigator: TezosNavigator) -> None:
    """Test the SETUP instruction."""
    snap_path = Path(f"{account}")

    main_chain_id = "NetXH12AexHqTQa" # Chain = 1
    main_hwm = Hwm(1, 0)
    test_hwm = Hwm(2, 0)

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
        test_hwm=test_hwm
    )


@pytest.mark.parametrize("account", ACCOUNTS)
def test_get_main_hwm(
        account: Account,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the QUERY_MAIN_HWM instruction."""

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on preattestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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

    with tezos_navigator.goto_home_hwm():
        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if not with_hash:
            signature = client.sign_message(account, preattestation)
            account.check_signature(signature, bytes(preattestation))
        else:
            preattestation_hash, signature = client.sign_message_with_hash(account, preattestation)
            assert preattestation_hash == preattestation.hash, \
                f"Expected hash {preattestation.hash.hex()} but got {preattestation_hash.hex()}"
            account.check_signature(signature, bytes(preattestation))

        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if firmware.is_nano:
            # No update for Stax or flex
            backend.both_click()
            backend.wait_for_screen_change()
        tezos_navigator.assert_screen("hwm_after_sign", snap_path=snap_path)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0)
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation(
        account: Account,
        with_hash: bool,
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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

    with tezos_navigator.goto_home_hwm():
        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if not with_hash:
            signature = client.sign_message(account, attestation)
            account.check_signature(signature, bytes(attestation))
        else:
            attestation_hash, signature = client.sign_message_with_hash(account, attestation)
            assert attestation_hash == attestation.hash, \
                f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
            account.check_signature(signature, bytes(attestation))

        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if firmware.is_nano:
            # No update for Stax or flex
            backend.both_click()
            backend.wait_for_screen_change()
        tezos_navigator.assert_screen("hwm_after_sign", snap_path=snap_path)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0)
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_attestation_dal(
        account: Account,
        with_hash: bool,
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on attestation."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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

    with tezos_navigator.goto_home_hwm():
        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if not with_hash:
            signature = client.sign_message(account, attestation)
            account.check_signature(signature, bytes(attestation))
        else:
            attestation_hash, signature = client.sign_message_with_hash(account, attestation)
            assert attestation_hash == attestation.hash, \
                f"Expected hash {attestation.hash.hex()} but got {attestation_hash.hex()}"
            account.check_signature(signature, bytes(attestation))

        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if firmware.is_nano:
            # No update for Stax or flex
            backend.both_click()
            backend.wait_for_screen_change()
        tezos_navigator.assert_screen("hwm_after_sign", snap_path=snap_path)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0)
    )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_block(
        account: Account,
        with_hash: bool,
        firmware: Firmware,
        backend: BackendInterface,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on block."""
    snap_path = Path(f"{account}")

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(0, 0)
    test_hwm = Hwm(0, 0)

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

    with tezos_navigator.goto_home_hwm():
        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if not with_hash:
            signature = client.sign_message(account, block)
            account.check_signature(signature, bytes(block))
        else:
            block_hash, signature = client.sign_message_with_hash(account, block)
            assert block_hash == block.hash, \
                f"Expected hash {block.hash.hex()} but got {block_hash.hex()}"
            account.check_signature(signature, bytes(block))

        tezos_navigator.assert_screen("hwm_before_sign", snap_path=snap_path)

        if firmware.is_nano:
            # No update for Stax or flex
            backend.both_click()
            backend.wait_for_screen_change()
        tezos_navigator.assert_screen("hwm_after_sign", snap_path=snap_path)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 2),
        test_hwm=Hwm(0, 0)
    )


def test_sign_block_at_reset_level(client: TezosClient, tezos_navigator: TezosNavigator) -> None:
    """Test that signing block at reset level fails."""

    account = DEFAULT_ACCOUNT

    reset_level: int = 1

    main_chain_id = DEFAULT_CHAIN_ID
    main_hwm = Hwm(reset_level, 0)
    test_hwm = Hwm(0, 0)

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


PARAMETERS_SIGN_LEVEL_AUTHORIZED = [
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
    PARAMETERS_SIGN_LEVEL_AUTHORIZED)
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
        main_hwm=Hwm(main_level, 0),
        test_hwm=Hwm(0, 0)
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


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_delegation(
        account: Account,
        with_hash: bool,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on delegation."""
    snap_path = Path(f"{account}")

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    delegation = Delegation(
        delegate=account.public_key_hash,
        source=account.public_key_hash,
    )

    raw_delegation = delegation.forge()

    if not with_hash:
        signature = tezos_navigator.sign_delegation(
            account,
            delegation,
            snap_path=snap_path
        )
        account.check_signature(signature, bytes(raw_delegation))
    else:
        delegation_hash, signature = \
            tezos_navigator.sign_delegation_with_hash(
                account,
                delegation,
                snap_path=snap_path
            )
        assert delegation_hash == raw_delegation.hash, \
            f"Expected hash {raw_delegation.hash.hex()} but got {delegation_hash.hex()}"
        account.check_signature(signature, bytes(raw_delegation))



PARAMETERS_SIGN_DELEGATION_FEES = [
    1,
    20000,
    300000000,
    50000060000,
    789789789
]

@pytest.mark.parametrize("fee", PARAMETERS_SIGN_DELEGATION_FEES)
def test_sign_delegation_fee(
        fee: int,
        tezos_navigator: TezosNavigator) -> None:
    """Test fee display on delegation."""

    account = DEFAULT_ACCOUNT
    snap_path = Path(f"fee_{fee}")

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    delegation = Delegation(
        delegate=account.public_key_hash,
        source=account.public_key_hash,
        fee=fee,
    )

    tezos_navigator.sign_delegation(
            account,
            delegation,
            snap_path=snap_path
        )


PARAMETERS_SIGN_DELEGATION_CONSTRAINTS = [
    (
        DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT,
        StatusCode.SECURITY
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT,
        StatusCode.SECURITY
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT,
        # Warning: operation PARSE_ERROR are not available on DEBUG-mode
        StatusCode.PARSE_ERROR
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2,
        # Warning: operation PARSE_ERROR are not available on DEBUG-mode
        StatusCode.PARSE_ERROR
    )
]

@pytest.mark.parametrize(
    "setup_account," \
    "delegate_account," \
    "source_account," \
    "signer_account," \
    "status_code",
    PARAMETERS_SIGN_DELEGATION_CONSTRAINTS
)
def test_sign_delegation_constraints(
        setup_account: Account,
        delegate_account: Account,
        source_account: Account,
        signer_account: Account,
        status_code: StatusCode,
        tezos_navigator: TezosNavigator) -> None:
    """Test delegation signing constraints."""

    tezos_navigator.setup_app_context(
        setup_account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    delegation = Delegation(
        delegate=delegate_account.public_key_hash,
        source=source_account.public_key_hash,
    )

    with status_code.expected():
        tezos_navigator.sign_delegation(
            signer_account,
            delegation
        )


@pytest.mark.parametrize("account", ACCOUNTS)
@pytest.mark.parametrize("with_hash", [False, True])
def test_sign_reveal(
        account: Account,
        with_hash: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test the SIGN(_WITH_HASH) instruction on reveal."""

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    reveal = Reveal(
        public_key=account.public_key,
        source=account.public_key_hash,
    ).forge()

    if not with_hash:
        signature = client.sign_message(
            account,
            reveal
        )
        account.check_signature(signature, bytes(reveal))
    else:
        reveal_hash, signature = \
            client.sign_message_with_hash(
                account,
                reveal
            )
        assert reveal_hash == reveal.hash, \
            f"Expected hash {reveal.hash.hex()} but got {reveal_hash.hex()}"
        account.check_signature(signature, bytes(reveal))


# Warning: operation PARSE_ERROR are not available on DEBUG-mode
PARAMETERS_SIGN_REVEAL_CONSTRAINTS = [
    (
        DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT,
        StatusCode.SECURITY
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT,
        StatusCode.PARSE_ERROR
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2, DEFAULT_ACCOUNT,
        StatusCode.PARSE_ERROR
    ),
    (
        DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT, DEFAULT_ACCOUNT_2,
        StatusCode.PARSE_ERROR
    )
]

@pytest.mark.parametrize(
    "setup_account," \
    "public_key_account," \
    "source_account," \
    "signer_account," \
    "status_code",
    PARAMETERS_SIGN_REVEAL_CONSTRAINTS
)
def test_sign_reveal_constraints(
        setup_account: Account,
        public_key_account: Account,
        source_account: Account,
        signer_account: Account,
        status_code: StatusCode,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test reveal signing constraints."""

    tezos_navigator.setup_app_context(
        setup_account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    reveal = Reveal(
        public_key=public_key_account.public_key,
        source=source_account.public_key_hash,
    ).forge()

    with status_code.expected():
        client.sign_message(
            signer_account,
            reveal
        )


def test_sign_not_authorized_key(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing with a key different from the authorized key is not authorized."""

    account_1 = DEFAULT_ACCOUNT
    account_2 = DEFAULT_ACCOUNT_2

    main_chain_id = DEFAULT_CHAIN_ID

    tezos_navigator.setup_app_context(
        account_1,
        main_chain_id,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(0, 0, main_chain_id)

    with StatusCode.SECURITY.expected():
        client.sign_message(account_2, attestation)


def test_sign_transaction(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing a transaction is not allowed."""

    account_1 = DEFAULT_ACCOUNT
    account_2 = DEFAULT_ACCOUNT_2

    main_chain_id = DEFAULT_CHAIN_ID

    tezos_navigator.setup_app_context(
        account_1,
        main_chain_id,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    ctxt = pytezos.using()
    transaction = UnsafeOp(
        ctxt.transaction(
            source=account_1.public_key_hash,
            destination=account_2.public_key_hash,
            amount=10_000,
        )
    ).forge()

    with StatusCode.PARSE_ERROR.expected():
        client.sign_message(account_1, transaction)


def build_reveal(account: Account) -> Reveal:
    """Build a reveal."""
    return Reveal(
        public_key=account.public_key,
        source=account.public_key_hash,
    )
def build_delegation(account: Account) -> Delegation:
    """Build a delegation."""
    return Delegation(
        delegate=account.public_key_hash,
        source=account.public_key_hash,
    )
def build_transaction(account: Account) -> UnsafeOp:
    """Build a transaction."""
    ctxt = pytezos.using()
    return UnsafeOp(
        ctxt.transaction(
            source=account.public_key_hash,
            destination=DEFAULT_ACCOUNT_2.public_key_hash,
            amount=10_000,
        )
    )
def build_bad_reveal_1(account: Account) -> Reveal:
    """Build a bad reveal."""
    return Reveal(
        public_key=account.public_key,
        source=DEFAULT_ACCOUNT_2.public_key_hash,
    )
def build_bad_reveal_2(account: Account) -> Reveal:
    """Build an other bad reveal."""
    return Reveal(
        public_key=DEFAULT_ACCOUNT_2.public_key,
        source=account.public_key_hash,
    )
def build_bad_delegation_1(account: Account) -> Delegation:
    """Build a bad delegation."""
    return Delegation(
        delegate=account.public_key_hash,
        source=DEFAULT_ACCOUNT_2.public_key_hash,
    )
def build_bad_delegation_2(account: Account) -> Delegation:
    """Build a other bad delegation."""
    return Delegation(
        delegate=DEFAULT_ACCOUNT_2.public_key_hash,
        source=account.public_key_hash,
    )


# Warning: operation PARSE_ERROR are not available on DEBUG-mode
PARAMETERS_SIGN_MULTIPLE_OPERATIONS = [
    (build_reveal,       build_reveal,           None,              False, StatusCode.OK         ),
    (build_reveal,       build_delegation,       None,              True,  StatusCode.OK         ),
    (build_delegation,   build_reveal,           None,              True,  StatusCode.OK         ),
    (build_reveal,       build_delegation,       build_reveal,      True,  StatusCode.OK         ),
] + [
    (build_bad_reveal_1, build_reveal,           None,              False, StatusCode.PARSE_ERROR),
    (build_bad_reveal_1, build_delegation,       None,              True,  StatusCode.PARSE_ERROR),
    (build_bad_reveal_2, build_reveal,           None,              False, StatusCode.PARSE_ERROR),
    (build_bad_reveal_2, build_delegation,       None,              True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_bad_reveal_1,     None,              False, StatusCode.PARSE_ERROR),
    (build_delegation,   build_bad_reveal_1,     None,              True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_bad_reveal_2,     None,              False, StatusCode.PARSE_ERROR),
    (build_delegation,   build_bad_reveal_2,     None,              True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_bad_delegation_1, None,              True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_bad_delegation_2, None,              True,  StatusCode.SECURITY   ),
    (build_bad_delegation_1, build_reveal,       None,              True,  StatusCode.PARSE_ERROR),
    (build_bad_delegation_2, build_reveal,       None,              True,  StatusCode.SECURITY   ),
    (build_reveal,       build_transaction,      None,              False, StatusCode.PARSE_ERROR),
    (build_transaction,  build_reveal,           None,              False, StatusCode.PARSE_ERROR),
    (build_delegation,   build_delegation,       None,              True,  StatusCode.PARSE_ERROR),
    (build_delegation,   build_delegation,       None,              True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_delegation,       build_delegation,  True,  StatusCode.PARSE_ERROR),
    (build_delegation,   build_delegation,       build_reveal,      True,  StatusCode.PARSE_ERROR),
    (build_reveal,       build_delegation,       build_transaction, True,  StatusCode.PARSE_ERROR),
    (build_transaction,  build_delegation,       build_reveal,      True,  StatusCode.PARSE_ERROR),
]

@pytest.mark.parametrize(
    "operation_builder_1," \
    "operation_builder_2," \
    "operation_builder_3," \
    "operation_display," \
    "status_code",
    PARAMETERS_SIGN_MULTIPLE_OPERATIONS
)
def test_sign_multiple_operation(
        operation_builder_1: Callable[[Account], UnsafeOp],
        operation_builder_2: Callable[[Account], UnsafeOp],
        operation_builder_3: Optional[Callable[[Account], UnsafeOp]],
        status_code: StatusCode,
        operation_display: bool,
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Test multiple operations signing constraints."""

    account = DEFAULT_ACCOUNT

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    operation = operation_builder_1(account)
    operation = operation.merge(
        operation_builder_2(account)
    )
    if operation_builder_3 is not None:
        operation = operation.merge(
            operation_builder_3(account)
        )

    message = operation.forge()

    with status_code.expected():
        if operation_display:
            signature = send_and_navigate(
                send=lambda: client.sign_message(
                    account,
                    message
                ),
                navigate=tezos_navigator.accept_sign_navigate
            )
        else:
            signature = client.sign_message(
                account,
                message
            )
        account.check_signature(signature, bytes(message))


def test_sign_when_hwm_disabled(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing, when HWM is disabled, changes the main HWM."""

    account = DEFAULT_ACCOUNT

    tezos_navigator.disable_hwm()

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,  # Chain = 0
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        1, 0,
        DEFAULT_CHAIN_ID  # Chain = 0
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(1, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        2, 0,
        "NetXH12AexHqTQa"  # Chain = 1
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(2, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        2, 0,
        "NetXH12Af5mrXhq"  # Chain = 2
    )

    with StatusCode.WRONG_VALUES.expected():
        client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(2, 0),
        test_hwm=Hwm(0, 0)
    )


@pytest.mark.parametrize("exit_style", ["abruptly", "properly"])
def test_hwm_disabled_exit(client: TezosClient, tezos_navigator: TezosNavigator, exit_style, backend_name) -> None:
    """On device test to verify HWM settings operation. Can run the test with hwm setting enabled or disabled.
       When HWM is disabled, an abrupt power off will result in HWM reset to 0.
       Whwereas when HWM settings is enabled, the abrupt power off will not
       reset the HWM. With a proper exit, HWM will always be preserved."""
    # check if backend is speculos, then return .
    if backend_name == "speculos":
        assert True
        return

    account = DEFAULT_ACCOUNT

    hwm_input = input("Is hwm disabled ? (y/n)")
    hwm_disabled = False
    if hwm_input.find("y") != -1:
        hwm_disabled = True

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID,  # Chain = 0
        main_hwm = Hwm(0, 0),
        test_hwm = Hwm(0, 0)
    )
    for i in range(1, 11):
        attestation = build_attestation(i, 0, DEFAULT_CHAIN_ID)
        client.sign_message(account, attestation)
    main_hwm = Hwm(10,0)
    received_main_hwm = tezos_navigator.client.get_main_hwm()
    assert received_main_hwm == main_hwm, \
        f"Expected main hmw {main_hwm} but got {received_main_hwm}"

    expected_hwm = Hwm(10,0)
    if hwm_disabled and exit_style == "abruptly":
        expected_hwm.highest_level = 0
    output = "No"
    output = input(f"""
                    1. Now switch off the device {exit_style}.
                    2. Reconnect it to power source.
                    3. Open the baking app.
                    4. Check that HWM is {expected_hwm}.
                    Is it correct? (Yes/No)""")
    if output.find("y") == -1 and output.find("Y") == -1:
        assert False, "HWM disabled setting is not working correctly."


def test_sign_when_no_chain_setup(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing when no chain has been setup change main HWM."""

    account = DEFAULT_ACCOUNT

    tezos_navigator.setup_app_context(
        account,
        DEFAULT_CHAIN_ID, # Chain = 0
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        1, 0,
        DEFAULT_CHAIN_ID # Chain = 0
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(1, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        2, 0,
        "NetXH12AexHqTQa" # Chain = 1
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(2, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        2, 0,
        "NetXH12Af5mrXhq" # Chain = 2
    )

    with StatusCode.WRONG_VALUES.expected():
        client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=DEFAULT_CHAIN_ID,
        main_hwm=Hwm(2, 0),
        test_hwm=Hwm(0, 0)
    )


def test_sign_when_chain_is_setup(
        client: TezosClient,
        tezos_navigator: TezosNavigator) -> None:
    """Check that signing when chain has been setup change main HWM."""

    account = DEFAULT_ACCOUNT
    main_chain_id = "NetXH12AexHqTQa" # Chain = 1

    tezos_navigator.setup_app_context(
        account,
        main_chain_id,
        main_hwm=Hwm(0, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        1, 0,
        main_chain_id
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 0),
        test_hwm=Hwm(0, 0)
    )

    attestation = build_attestation(
        2, 0,
        DEFAULT_CHAIN_ID # Chain = 0
    )

    client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 0),
        test_hwm=Hwm(2, 0)
    )

    attestation = build_attestation(
        2, 0,
        "NetXH12Af5mrXhq" # Chain = 2
    )

    with StatusCode.WRONG_VALUES.expected():
        client.sign_message(account, attestation)

    tezos_navigator.check_app_context(
        account,
        chain_id=main_chain_id,
        main_hwm=Hwm(1, 0),
        test_hwm=Hwm(2, 0)
    )


KEY_SHA256_HEX = "6c4e7e706c54d367c87a8d89c16adfe06cb5680cb7d18e625a90475ec0dbdb9f"

def get_hmac_key(account):
    """Get the HMAC key."""
    digest = bytes.fromhex(KEY_SHA256_HEX)
    hmac_key = account.sign_prehashed_message(digest)
    return hashlib.sha512(hmac_key).digest()

HMAC_TEST_SET = [
    ("00"),
    ("000000000000000000000000000000000000000000000000"),
    ("0123456789abcdef"),
    ("0123456789abcdef0123456789abcdef0123456789abcdef")
]

# This HMAC test don't pass with tz2 and tz3
@pytest.mark.parametrize("account", TZ1_ACCOUNTS)
@pytest.mark.parametrize("message_hex", HMAC_TEST_SET)
def test_hmac(
        account: Account,
        message_hex: str,
        client: TezosClient) -> None:
    """Test the HMAC instruction."""

    message = bytes.fromhex(message_hex)

    hmac_key = get_hmac_key(account)

    calculated_hmac = hmac.digest(
        key=hmac_key,
        msg=message,
        digest=hashlib.sha256
    )

    data = client.hmac(account, message)
    assert hmac.compare_digest(calculated_hmac, data), \
        f"Expected HMAC {calculated_hmac.hex()} but got {data.hex()}"
