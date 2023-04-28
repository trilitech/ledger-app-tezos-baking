from pathlib import Path

from ragger.navigator import NavInsID, NavIns
from apps.tezos import TezosClient, StatusCode
from apps.tezos import TEZ_PACKED_DERIVATION_PATH, OPERATION_TAG
from utils import get_nano_review_instructions, get_stax_review_instructions, get_stax_address_instructions

import pytest

TESTS_ROOT_DIR = Path(__file__).parent


def test_get_public_key_silent(test_name, backend, firmware, navigator):

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


def test_sign_message_proposal(test_name, backend, firmware, navigator):
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH, OPERATION_TAG.OPERATION_TAG_PROPOSAL):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_message_ballot(test_name, backend, firmware, navigator):
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH, OPERATION_TAG.OPERATION_TAG_BALLOT):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


@pytest.mark.parametrize("operation", [OPERATION_TAG.OPERATION_TAG_BABYLON_REVEAL,
                                       OPERATION_TAG.OPERATION_TAG_ATHENS_REVEAL])
def test_sign_message_reveal(test_name, backend, firmware, navigator, operation):
    folder_name = test_name + "/" + str(operation)
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH, operation):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       folder_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_message_transaction(test_name, backend, firmware, navigator):
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH,
                          OPERATION_TAG.OPERATION_TAG_BABYLON_TRANSACTION):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_message_origination(test_name, backend, firmware, navigator):
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH, OPERATION_TAG.OPERATION_TAG_ATHENS_ORIGINATION):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)


def test_sign_message_delegation(test_name, backend, firmware, navigator):
    tez = TezosClient(backend)

    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(6)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(4)
    else:
        instructions = get_stax_review_instructions(2)

    with tez.sign_message(TEZ_PACKED_DERIVATION_PATH, OPERATION_TAG.OPERATION_TAG_BABYLON_DELEGATION):
        navigator.navigate_and_compare(TESTS_ROOT_DIR,
                                       test_name,
                                       instructions)

    response: bytes = tez.get_async_response()
    assert (response.status == StatusCode.STATUS_OK)
