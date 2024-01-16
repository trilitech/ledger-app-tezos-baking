"""Module gathering the baking app instruction tests."""

from pathlib import Path
from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from utils.client import TezosClient
from utils.client import TEZ_PACKED_DERIVATION_PATH
from utils.helper import (
    get_nano_review_instructions,
    get_stax_review_instructions,
    get_stax_address_instructions
)

TESTS_ROOT_DIR = Path(__file__).parent


def test_reset_hwm(
        backend: BackendInterface,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the RESET instruction."""

    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(3)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_stax_review_instructions(2)

    reset_level: int = 0

    with tez.reset_app_context(reset_level):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)


def test_authorize_baking(
        backend: BackendInterface,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the AUTHORIZE_BAKING instruction."""

    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_address_instructions()

    with tez.authorize_baking(TEZ_PACKED_DERIVATION_PATH):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)


def test_get_public_key_baking(
        backend: BackendInterface,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_address_instructions()

    with tez.get_public_key_prompt(TEZ_PACKED_DERIVATION_PATH):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)


def test_setup_baking_address(
        backend: BackendInterface,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the SETUP instruction."""

    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(8)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(7)
    else:
        instructions = get_stax_review_instructions(2)

    chain: int = 0
    main_hwm: int = 0
    test_hwm: int = 0

    with tez.setup_baking_address(TEZ_PACKED_DERIVATION_PATH, chain, main_hwm, test_hwm):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)


def test_get_public_key_silent(backend: BackendInterface) -> None:
    """Test the GET_PUBLIC_KEY instruction."""

    tez = TezosClient(backend)

    with tez.get_public_key_silent(TEZ_PACKED_DERIVATION_PATH):
        pass


def test_get_public_key_prompt(
        backend: BackendInterface,
        firmware: Firmware,
        navigator: Navigator,
        test_name: Path) -> None:
    """Test the PROMPT_PUBLIC_KEY instruction."""

    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_address_instructions()

    with tez.get_public_key_prompt(TEZ_PACKED_DERIVATION_PATH):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)
