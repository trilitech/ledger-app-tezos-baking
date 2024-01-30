"""Module of test helper functions."""

from typing import TypeVar, Callable, List, Union

from multiprocessing.pool import ThreadPool
import time
import git

from ragger.firmware import Firmware
from ragger.navigator import NavInsID, NavIns

class BytesReader:
    """Class representing a bytes reader."""

    def __init__(self, data: bytes):
        self.data = data
        self.offset: int = 0

    def remaining_size(self) -> int:
        """Return the size of the remaining data to read."""
        return len(self.data) - self.offset

    def has_finished(self) -> bool:
        """Return if all the data has been read."""
        return self.remaining_size() == 0

    def assert_finished(self) -> None:
        """Assert all the data has been read."""
        assert self.has_finished(), \
            f"Should have been finished, but some data left: {self.data[self.offset:].hex()}"

    def read_int(self, size: int) -> int:
        """Read a size-byte-long integer."""
        raw_int = self.data[self.offset : self.offset+size]
        res = int.from_bytes(raw_int, byteorder='big')
        self.offset += size
        return res

    def read_bytes(self, size: int) -> bytes:
        """Read a size-byte-long bytes."""
        res = self.data[self.offset : self.offset+size]
        self.offset += size
        return res

def get_current_commit() -> str:
    """Return the current commit."""
    repo = git.Repo(".")
    commit = repo.head.commit.hexsha[:8]
    if repo.is_dirty():
        commit += "*"
    return commit

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
    def get_nano_review_instructions(num_screen_skip) -> List[Union[NavInsID, NavIns]]:
        """Generate the instructions needed to review on nano devices."""
        instructions: List[Union[NavInsID, NavIns]] = []
        instructions += [NavInsID.RIGHT_CLICK] * num_screen_skip
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
