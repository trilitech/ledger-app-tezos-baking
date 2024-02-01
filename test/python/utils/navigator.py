"""Module providing a tezos navigator."""

from pathlib import Path
from typing import TypeVar, Callable, List, Optional, Union
import time

from multiprocessing.pool import ThreadPool

from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.firmware.stax.screen import MetaScreen
from ragger.firmware.stax.use_cases import (
    UseCaseHome,
    UseCaseSettings,
    UseCaseReview,
    UseCaseAddressConfirmation
)
from ragger.navigator import Navigator, NavInsID, NavIns

from common import TESTS_ROOT_DIR, EMPTY_PATH
from utils.client import TezosClient, Hwm
from utils.account import Account

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

class Instructions:
    """Class gathering instructions generator needed for navigator."""

    @staticmethod
    def get_right_clicks(nb_right_click) -> List[Union[NavInsID, NavIns]]:
        """Generate `nb_right_click` right clicks instructions."""
        return [NavInsID.RIGHT_CLICK] * nb_right_click

    @staticmethod
    def get_nano_review_instructions(num_screen_skip) -> List[Union[NavInsID, NavIns]]:
        """Generate the instructions needed to review on nano devices."""
        instructions = Instructions.get_right_clicks(num_screen_skip)
        instructions.append(NavInsID.BOTH_CLICK)
        return instructions

    @staticmethod
    def get_stax_review_instructions(num_screen_skip) -> List[Union[NavInsID, NavIns]]:
        """Generate the instructions needed to review on stax devices."""
        instructions: List[Union[NavInsID, NavIns]] = []
        instructions += [NavInsID.USE_CASE_REVIEW_TAP] * num_screen_skip
        instructions += [
            NavInsID.USE_CASE_CHOICE_CONFIRM,
            NavInsID.USE_CASE_STATUS_DISMISS
        ]
        return instructions

    @staticmethod
    def get_stax_address_instructions() -> List[Union[NavInsID, NavIns]]:
        """Generate the instructions needed to check address on stax devices."""
        instructions: List[Union[NavInsID, NavIns]] = [
            NavInsID.USE_CASE_REVIEW_TAP,
            NavIns(NavInsID.TOUCH, (112, 251)),
            NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
            NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
            NavInsID.USE_CASE_STATUS_DISMISS
        ]
        return instructions

    @staticmethod
    def get_public_key_flow_instructions(firmware: Firmware):
        """Generate the instructions needed to check address."""
        if firmware.device == "nanos":
            return Instructions.get_nano_review_instructions(5)
        if firmware.is_nano:
            return Instructions.get_nano_review_instructions(4)
        return Instructions.get_stax_address_instructions()

    @staticmethod
    def get_setup_app_context_instructions(firmware: Firmware):
        """Generate the instructions needed to setup app context."""
        if firmware.device == "nanos":
            return Instructions.get_nano_review_instructions(8)
        if firmware.is_nano:
            return Instructions.get_nano_review_instructions(7)
        return Instructions.get_stax_review_instructions(2)

    @staticmethod
    def get_reset_app_context_instructions(firmware: Firmware):
        """Generate the instructions needed to reset app context."""
        if firmware.device == "nanos":
            return Instructions.get_nano_review_instructions(3)
        if firmware.is_nano:
            return Instructions.get_nano_review_instructions(3)
        return Instructions.get_stax_review_instructions(2)

class TezosNavigator(metaclass=MetaScreen):
    """Class representing the tezos app navigator."""

    use_case_home       = UseCaseHome
    use_case_settings   = UseCaseSettings
    use_case_review     = UseCaseReview
    use_case_provide_pk = UseCaseAddressConfirmation

    home:       UseCaseHome
    settings:   UseCaseSettings
    review:     UseCaseReview
    provide_pk: UseCaseAddressConfirmation

    backend:   BackendInterface
    firmware:  Firmware
    client:    TezosClient
    navigator: Navigator

    _golden_run:        bool
    _root_dir:          Path
    _snapshots_dir:     Path
    _tmp_snapshots_dir: Path

    def __init__(self, backend, firmware, client, navigator, golden_run) -> None:
        self.backend   = backend
        self.firmware  = firmware
        self.client    = client
        self.navigator = navigator

        self._golden_run        = golden_run
        self._root_dir          = TESTS_ROOT_DIR
        self._snapshots_dir     = self._root_dir / "snapshots" / backend.firmware.name
        self._tmp_snapshots_dir = self._root_dir / "snapshots-tmp" / backend.firmware.name

    def _send_and_navigate(self,
                          send: Callable[[], RESPONSE],
                          instructions: List[Union[NavInsID, NavIns]],
                          path: Optional[Path] = None,
                          **kwargs) -> RESPONSE:
        return send_and_navigate(
            send=send,
            navigate=lambda: self.navigator.navigate_and_compare(
                self._root_dir,
                path,
                instructions,
                **kwargs
            )
        )

    def assert_screen(self, path: Path) -> None:
        golden_path = self._snapshots_dir / path
        if not golden_path.parent.is_dir() and self._golden_run:
            golden_path.parent.mkdir(parents=True)
        tmp_path = self._tmp_snapshots_dir / path
        if not tmp_path.parent.is_dir():
            tmp_path.parent.mkdir(parents=True)

        assert self.backend.compare_screen_with_snapshot(
            golden_path,
            tmp_snap_path=tmp_path,
            golden_run=self._golden_run
        ), f"Screen does not match golden {path}."

    def check_app_context(self,
                          account: Optional[Account],
                          chain_id: str,
                          main_hwm: Hwm,
                          test_hwm: Hwm,
                          path: Path) -> None:
        """Check that the app context."""
        path = Path(path)

        received_chain_id, received_main_hwm, received_test_hwm = self.client.get_all_hwm()

        # get_auth_key_with_curve raise EXC_REFERENCED_DATA_NOT_FOUND
        received_path = self.client.get_auth_key()

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

        if self.firmware.is_nano:
            self.navigator.navigate(
                [NavInsID.RIGHT_CLICK] * 2,
                screen_change_before_first_instruction=False
            )
            self.assert_screen(path / "chain_id.png")
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            if account is not None and self.firmware.device == "nanos":
                self.assert_screen(path / "public_key_hash_1.png")
                self.backend.right_click()
                self.backend.wait_for_screen_change()
                self.assert_screen(path / "public_key_hash_2.png")
            else:
                self.assert_screen(path / "public_key_hash.png")
            self.backend.right_click()
            self.backend.wait_for_screen_change()
            self.assert_screen(path / "high_watermark.png")
            self.navigator.navigate(
                [NavInsID.LEFT_CLICK] * 4,
                screen_change_before_first_instruction=False
            )
        else:
            self.home.settings()
            self.backend.wait_for_screen_change()
            self.assert_screen(path / "app_context.png")
            self.settings.multi_page_exit()
            self.backend.wait_for_screen_change()

    def authorize_baking(self, account: Optional[Account], **kwargs) -> bytes:
        """Send an authorize baking request and navigate until accept"""
        if 'instructions' not in kwargs:
            kwargs['instructions'] = \
                Instructions.get_public_key_flow_instructions(self.firmware)
        return self._send_and_navigate(
            send=lambda: self.client.authorize_baking(account),
            **kwargs
        )

    def get_public_key_prompt(self, account: Account, **kwargs) -> bytes:
        """Send a get public key request and navigate until accept"""
        if 'instructions' not in kwargs:
            kwargs['instructions'] = \
                Instructions.get_public_key_flow_instructions(self.firmware)
        return self._send_and_navigate(
            send=lambda: self.client.get_public_key_prompt(account),
            **kwargs
        )

    def reset_app_context(self, reset_level: int, **kwargs) -> None:
        """Send a reset request and navigate until accept"""
        if 'instructions' not in kwargs:
            kwargs['instructions'] = \
                Instructions.get_reset_app_context_instructions(self.firmware)
        return self._send_and_navigate(
            send=lambda: self.client.reset_app_context(reset_level),
            **kwargs
        )

    def setup_app_context(self,
                          account: Account,
                          main_chain_id: str,
                          main_hwm: Hwm,
                          test_hwm: Hwm,
                          **kwargs) -> bytes:
        """Send a setup request and navigate until accept"""
        if 'instructions' not in kwargs:
            kwargs['instructions'] = \
                Instructions.get_setup_app_context_instructions(self.firmware)
        return self._send_and_navigate(
            send=lambda: self.client.setup_app_context(
                account,
                main_chain_id,
                main_hwm,
                test_hwm
            ),
            **kwargs
        )
