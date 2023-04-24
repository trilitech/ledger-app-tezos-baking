from typing import Optional, Tuple
from pathlib import Path
from bip32 import HARDENED_INDEX
from enum import IntEnum

from ragger.backend import SpeculosBackend
from ragger.backend.interface import RAPDU

from ragger.navigator import NavInsID, NavIns
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.backends import default_backend


def app_path_from_app_name(app_dir, app_name: str, device: str) -> Path:
    assert app_dir.is_dir(), f"{app_dir} is not a directory"
    app_path = app_dir / (app_name + "_" + device + ".elf")
    assert app_path.is_file(), f"{app_path} must exist"
    return app_path


def prefix_with_len(to_prefix: bytes) -> bytes:
    return len(to_prefix).to_bytes(1, byteorder="big") + to_prefix


def validate_displayed_message(client: SpeculosBackend, num_screen_skip: int):
    for _ in range(num_screen_skip):
        if client.firmware.device == "fat":
            client.finger_touch(55, 550)
        else:
            client.right_click()


def approve_message(client: SpeculosBackend):
    if client.firmware.device == "fat":
        client.finger_touch(55, 550, 3)
    else:
        client.both_click()


def get_async_response(client: SpeculosBackend) -> RAPDU:
    return client.last_async_response


# DERIVATION PATHS CALCULATIONS

def pack_derivation_path(derivation_path: str) -> bytes:
    split = derivation_path.split("/")
    assert split.pop(0) == "m", "master expected"
    derivation_path: bytes = (len(split)).to_bytes(1, byteorder='big')
    for i in split:
        if (i[-1] == "'"):
            derivation_path += (int(i[:-1]) |
                                HARDENED_INDEX).to_bytes(4, byteorder='big')
        else:
            derivation_path += int(i).to_bytes(4, byteorder='big')
    return derivation_path


def get_nano_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions


def get_stax_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)] * num_screen_skip
    instructions.append(NavIns(NavInsID.USE_CASE_CHOICE_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_DISMISS))
    return instructions


def get_stax_address_instructions():
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)]
    instructions.append(NavIns(NavInsID.TOUCH, (112, 251)))
    instructions.append(NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR))
    instructions.append(NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_DISMISS))
    return instructions
