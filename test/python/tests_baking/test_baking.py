from pathlib import Path

from apps.tezos import TezosClient, StatusCode
from apps.tezos import TEZ_PACKED_DERIVATION_PATH
from utils import get_nano_review_instructions, get_stax_review_instructions, get_stax_address_instructions

TESTS_ROOT_DIR = Path(__file__).parent


def test_reset_HMW(test_name, backend, firmware, navigator):

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

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_authorize_baking(test_name, backend, firmware, navigator):

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

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_get_public_key_baking(test_name, backend, firmware, navigator):

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

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_setup_baking_address(test_name, backend, firmware, navigator):

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

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_get_public_key_silent(backend):

    tez = TezosClient(backend)

    with tez.get_public_key_silent(TEZ_PACKED_DERIVATION_PATH):
        pass

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_get_public_key_prompt(test_name, backend, firmware, navigator):

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

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)
