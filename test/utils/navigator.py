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

"""Module providing a tezos navigator."""

from pathlib import Path
from typing import TypeVar, Callable, Optional, Tuple, Union, Generator
import time

from contextlib import contextmanager
from multiprocessing.pool import ThreadPool

from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.firmware.touch.screen import MetaScreen
from ragger.firmware.touch.layouts import ChoiceList
from ragger.firmware.touch.use_cases import (
    UseCaseHome,
    UseCaseSettings,
    UseCaseReview,
    UseCaseAddressConfirmation
)
from ragger.firmware.touch.positions import (
    Position,
    STAX_CENTER
)
from ragger.navigator import Navigator, NavInsID, NavIns

from common import TESTS_ROOT_DIR, EMPTY_PATH
from utils.client import TezosClient, Hwm
from utils.account import Account, Signature
from utils.message import (
    Delegation,
    DEFAULT_BLOCK_HASH
)

RESPONSE = TypeVar('RESPONSE')

def send_and_navigate(send: Callable[[], RESPONSE], navigate: Callable[[], None]) -> RESPONSE:
    """Sends a request and navigates before receiving a response."""

    with ThreadPool(processes=2) as pool:

        send_res = pool.apply_async(send)
        navigate_res = pool.apply_async(navigate)

        while True:
            if send_res.ready():
                result = send_res.get()
                navigate_res.get()
                break
            if navigate_res.ready():
                navigate_res.get()
                result = send_res.get()
                break
            time.sleep(0.1)

        return result

class TezosUseCaseAddressConfirmation(UseCaseAddressConfirmation):
    """Extension of UseCaseAddressConfirmation for our app."""

    @property
    def qr_position(self) -> Position:
        """Position of the qr code.
        Y-285 = common space shared by buttons under a key displayed on 2 or 3 lines
        """
        return Position(STAX_CENTER.x, 285)

    def show_qr(self) -> None:
        """Tap to show qr code."""
        self.client.finger_touch(*self.qr_position)


APP_CONTEXT = Path("app_context")


class TezosNavigator(metaclass=MetaScreen):
    """Class representing the tezos app navigator."""

    use_case_home       = UseCaseHome
    use_case_settings   = UseCaseSettings
    use_case_review     = UseCaseReview
    use_case_provide_pk = TezosUseCaseAddressConfirmation
    layout_choice_list = ChoiceList

    home:       UseCaseHome
    settings:   UseCaseSettings
    review:     UseCaseReview
    provide_pk: TezosUseCaseAddressConfirmation
    layout_choice: ChoiceList

    backend:   BackendInterface
    firmware:  Firmware
    client:    TezosClient
    navigator: Navigator

    _golden_run:        bool
    _root_dir:          Path
    _test_name:         str
    _snapshots_dir:     Path
    _tmp_snapshots_dir: Path

    def __init__(self,
                 backend: BackendInterface,
                 firmware: Firmware,
                 client: TezosClient,
                 navigator: Navigator,
                 golden_run: bool,
                 test_name: str) -> None:
        self.backend   = backend
        self.firmware  = firmware
        self.client    = client
        self.navigator = navigator
        self.layout_choice = ChoiceList(backend, firmware)

        self._golden_run        = golden_run
        self._root_dir          = TESTS_ROOT_DIR
        self._test_name         = test_name
        self._snapshots_dir     = \
            self._root_dir / "snapshots" / self.firmware.name / self._test_name
        self._tmp_snapshots_dir = \
            self._root_dir / "snapshots-tmp" / self.firmware.name / self._test_name

    def navigate_and_compare(self,
                             snap_path: Optional[Union[Path, str]] = None,
                             **kwargs) -> None:
        """Same as navigator.navigate_and_compare"""
        if snap_path is not None:
            snap_path = Path(self._test_name) / snap_path
        if 'instructions' in kwargs:
            self.navigator.navigate_and_compare(
                path=self._root_dir,
                test_case_name=snap_path,
                **kwargs
            )
        else:
            # Need 'navigate_instruction', 'validation_instructions' and 'text' in kwargs
            self.navigator.navigate_until_text_and_compare(
                path=self._root_dir,
                test_case_name=snap_path,
                **kwargs
            )

    @staticmethod
    def _can_navigate(**kwargs) -> bool:
        can_navigate = 'instructions' in kwargs
        can_navigate_until = \
            'navigate_instruction' in kwargs and \
            'validation_instructions' in kwargs and \
            'text' in kwargs
        return can_navigate or can_navigate_until

    def assert_screen(self,
                      name: str,
                      snap_path: Path = Path("")) -> None:
        """Assert the current screen is the golden snap_path."""

        snap_path = snap_path / f"{name}.png"
        golden_path = self._snapshots_dir / snap_path
        if not golden_path.parent.is_dir() and self._golden_run:
            golden_path.parent.mkdir(parents=True)
        tmp_path = self._tmp_snapshots_dir / snap_path
        if not tmp_path.parent.is_dir():
            tmp_path.parent.mkdir(parents=True)

        assert self.backend.compare_screen_with_snapshot(
            golden_path,
            tmp_snap_path=tmp_path,
            golden_run=self._golden_run
        ), f"Screen does not match golden {snap_path}."

    def check_app_context(self,
                          account: Optional[Account],
                          chain_id: str,
                          main_hwm: Hwm,
                          test_hwm: Hwm,
                          snap_path: Path = Path(""),
                          backend_name = "speculos") -> None:
        """Check that the app context."""

        received_chain_id, received_main_hwm, received_test_hwm = self.client.get_all_hwm()

        assert received_chain_id == chain_id, \
            f"Expected main chain id {chain_id} but got {received_chain_id}"
        assert received_main_hwm == main_hwm, \
            f"Expected main hmw {main_hwm} but got {received_main_hwm}"
        assert received_test_hwm == test_hwm, \
            f"Expected test hmw {test_hwm} but got {received_test_hwm}"

        if account is None:
            # get_auth_key_with_curve raise EXC_REFERENCED_DATA_NOT_FOUND
            received_path = self.client.get_auth_key()
            assert received_path == EMPTY_PATH, \
                f"Expected the empty path {EMPTY_PATH} but got {received_path}"
        else:
            received_sig_scheme, received_path = self.client.get_auth_key_with_curve()
            assert received_path == account.path, \
                f"Expected path {account.path} but got {received_path}"
            assert received_sig_scheme == account.sig_scheme, \
                f"Expected signature scheme {account.sig_scheme.name} "\
                f"but got {received_sig_scheme.name}"

        if backend_name != "speculos":
            return

        snap_path = snap_path / APP_CONTEXT
        if self.firmware.is_nano:
            self.backend.wait_for_home_screen()
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("version", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("chain_id", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            if account is not None and self.firmware.device == "nanos":
                for i in range(1, account.nanos_screens + 1):
                    self.assert_screen("public_key_hash_" + str(i), snap_path=snap_path)
                    self.backend.right_click()
                    self.backend.wait_for_screen_change()
            else:
                self.assert_screen("public_key_hash", snap_path=snap_path)
                self.backend.right_click()
                self.backend.wait_for_screen_change()
            self.assert_screen("high_watermark", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("settings", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("exit", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)
        else:
            self.assert_screen("home_screen", snap_path=snap_path)
            self.home.settings()
            self.backend.wait_for_screen_change()
            self.assert_screen("app_context", snap_path=snap_path)
            self.settings.next()
            self.backend.wait_for_screen_change()
            self.assert_screen("hwm_status", snap_path=snap_path)
            self.settings.next()
            self.backend.wait_for_screen_change()
            self.assert_screen("description", snap_path=snap_path)
            self.settings.multi_page_exit()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)

    @contextmanager
    def goto_home_public_key(self, snap_path: Path = Path("")) -> Generator[None, None, None]:
        snap_path = snap_path / APP_CONTEXT

        if self.firmware.is_nano:
            self.backend.wait_for_home_screen()
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("version", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("chain_id", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
        else:
            self.assert_screen("home_screen", snap_path=snap_path)
            self.home.settings()
            self.backend.wait_for_screen_change()

        yield

        if self.firmware.is_nano:
            self.backend.left_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("chain_id", snap_path=snap_path)
            self.backend.left_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("version", snap_path=snap_path)
            self.backend.left_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)
        else:
            self.settings.multi_page_exit()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)

    @contextmanager
    def goto_home_hwm(self, snap_path: Path = Path("")) -> Generator[None, None, None]:
        snap_path = snap_path / APP_CONTEXT

        if self.firmware.is_nano:
            self.backend.wait_for_home_screen()
            self.backend.left_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("exit", snap_path=snap_path)
            self.backend.left_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("settings", snap_path=snap_path)
            self.backend.left_click()
            self.backend.wait_for_screen_change()
        else:
            self.assert_screen("home_screen", snap_path=snap_path)
            self.home.settings()
            self.backend.wait_for_screen_change()

        yield

        if self.firmware.is_nano:
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("settings", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("exit", snap_path=snap_path)
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)
        else:
            self.settings.multi_page_exit()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen", snap_path=snap_path)

    def accept_key_navigate(self, **kwargs):
        """Navigate until accept key"""
        if self.firmware.is_nano:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.RIGHT_CLICK,
                validation_instructions = [NavInsID.BOTH_CLICK],
                text = 'Accept',
                **kwargs
            )
        else:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.USE_CASE_ADDRESS_CONFIRMATION_TAP,
                validation_instructions = [
                    NavIns(NavInsID.TOUCH, self.provide_pk.qr_position),
                    NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                    NavInsID.USE_CASE_CHOICE_CONFIRM,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ],
                text = 'Confirm',
                **kwargs
            )

    def authorize_baking(self,
                         account: Optional[Account],
                         navigate: Optional[Callable] = None,
                         **kwargs) -> bytes:
        """Send an authorize baking request and navigate until accept"""
        if navigate is None:
            navigate = self.accept_key_navigate
        return send_and_navigate(
            send=lambda: self.client.authorize_baking(account),
            navigate=lambda: navigate(**kwargs)
        )

    def get_public_key_prompt(self,
                              account: Account,
                              navigate: Optional[Callable] = None,
                              **kwargs) -> bytes:
        """Send a get public key request and navigate until accept"""
        if navigate is None:
            navigate = self.accept_key_navigate
        return send_and_navigate(
            send=lambda: self.client.get_public_key_prompt(account),
            navigate=lambda: navigate(**kwargs)
        )

    def accept_reset_navigate(self, **kwargs):
        """Navigate until accept reset"""
        if self.firmware.is_nano:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.RIGHT_CLICK,
                validation_instructions = [NavInsID.BOTH_CLICK],
                text = 'Accept',
                **kwargs
            )
        else:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.USE_CASE_REVIEW_TAP,
                validation_instructions = [
                    NavInsID.USE_CASE_CHOICE_CONFIRM,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ],
                text = 'Approve',
                **kwargs
            )

    def reset_app_context(self,
                          reset_level: int,
                          navigate: Optional[Callable] = None,
                          **kwargs) -> None:
        """Send a reset request and navigate until accept"""
        if navigate is None:
            navigate = self.accept_reset_navigate
        return send_and_navigate(
            send=lambda: self.client.reset_app_context(reset_level),
            navigate=lambda: navigate(**kwargs)
        )

    def accept_setup_navigate(self, **kwargs):
        """Navigate until accept setup"""
        if self.firmware.is_nano:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.RIGHT_CLICK,
                validation_instructions = [NavInsID.BOTH_CLICK],
                text = 'Accept',
                **kwargs
            )
        else:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.USE_CASE_REVIEW_TAP,
                validation_instructions = [
                    NavInsID.USE_CASE_CHOICE_CONFIRM,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ],
                text = 'Approve',
                **kwargs
            )

    def setup_app_context(self,
                          account: Account,
                          main_chain_id: str,
                          main_hwm: Hwm,
                          test_hwm: Hwm,
                          navigate: Optional[Callable] = None,
                          **kwargs) -> bytes:
        """Send a setup request and navigate until accept"""
        if navigate is None:
            navigate = self.accept_setup_navigate
        return send_and_navigate(
            send=lambda: self.client.setup_app_context(
                account,
                main_chain_id,
                main_hwm,
                test_hwm
            ),
            navigate=lambda: navigate(**kwargs)
        )

    def accept_sign_navigate(self, **kwargs):
        """Navigate until accept signing"""
        if self.firmware.is_nano:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.RIGHT_CLICK,
                validation_instructions = [NavInsID.BOTH_CLICK],
                text = 'Accept',
                **kwargs
            )
        else:
            self.navigate_and_compare(
                navigate_instruction = NavInsID.USE_CASE_REVIEW_TAP,
                validation_instructions = [
                    NavInsID.USE_CASE_CHOICE_CONFIRM,
                    NavInsID.USE_CASE_STATUS_DISMISS
                ],
                text = 'Approve',
                **kwargs
            )

    def sign_delegation(self,
                        account: Account,
                        delegation: Delegation,
                        branch: str = DEFAULT_BLOCK_HASH,
                        navigate: Optional[Callable] = None,
                        **kwargs) -> Signature:
        """Send a sign request on delegation and navigate until accept"""
        if navigate is None:
            navigate = self.accept_sign_navigate
        return send_and_navigate(
            send=lambda: self.client.sign_message(
                account,
                delegation.forge(branch)
            ),
            navigate=lambda: navigate(**kwargs)
        )

    def sign_delegation_with_hash(self,
                                  account: Account,
                                  delegation: Delegation,
                                  branch: str = DEFAULT_BLOCK_HASH,
                                  navigate: Optional[Callable] = None,
                                  **kwargs) -> Tuple[bytes, Signature]:
        """Send a sign and get hash request on delegation and navigate until accept"""
        if navigate is None:
            navigate = self.accept_sign_navigate
        return send_and_navigate(
            send=lambda: self.client.sign_message_with_hash(
                account,
                delegation.forge(branch)
            ),
            navigate=lambda: navigate(**kwargs)
        )

    def right(self):
        """Move to right screen"""
        self.backend.right_click()
        self.backend.wait_for_screen_change()

    def left(self):
        """Move to left screen"""
        self.backend.left_click()
        self.backend.wait_for_screen_change()

    def press_both_buttons(self):
        """Press both buttons in Nano device"""
        self.backend.both_click()
        self.backend.wait_for_screen_change()

    def disable_hwm(self, snap_path: Path) -> None:
        """Disables HWM settings by navigating on the screen starting from home_screen"""
        if self.firmware.is_nano:
            self.assert_screen("home_screen",snap_path)
            self.left()
            self.assert_screen("quit",snap_path)
            self.left()
            self.assert_screen("Settings",snap_path)
            self.press_both_buttons()
            self.assert_screen("hwm_enabled",snap_path)
            self.press_both_buttons()
            self.assert_screen("hwm_disabled",snap_path)
            self.right()
            self.assert_screen("back",snap_path)
            self.press_both_buttons()
            self.assert_screen("home_screen",snap_path)
        else:
            self.backend.wait_for_home_screen()
            self.home.settings()
            self.backend.wait_for_screen_change()
            self.assert_screen("app_context",snap_path)
            self.settings.next()
            self.backend.wait_for_screen_change()
            self.assert_screen("hwm_status_on",snap_path)
            self.layout_choice.choose(1)
            self.backend.wait_for_screen_change()
            self.assert_screen("hwm_status_off",snap_path)
            self.settings.multi_page_exit()
            self.backend.wait_for_screen_change()
            self.assert_screen("home_screen",snap_path)
