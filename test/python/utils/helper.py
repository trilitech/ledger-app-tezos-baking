"""Module of test helper functions."""

from typing import List, Union

from ragger.navigator import NavInsID, NavIns

def get_nano_review_instructions(num_screen_skip) -> List[Union[NavInsID, NavIns]]:
    """Generate the instructions needed to review on nano devices."""
    instructions: List[Union[NavInsID, NavIns]] = []
    instructions += [NavInsID.RIGHT_CLICK] * num_screen_skip
    instructions.append(NavInsID.BOTH_CLICK)
    return instructions

def get_stax_review_instructions(num_screen_skip) -> List[Union[NavInsID, NavIns]]:
    """Generate the instructions needed to review on stax devices."""
    instructions: List[Union[NavInsID, NavIns]] = []
    instructions += [NavInsID.USE_CASE_REVIEW_TAP] * num_screen_skip
    instructions += [
        NavInsID.USE_CASE_CHOICE_CONFIRM,
        NavInsID.USE_CASE_STATUS_DISMISS
    ]
    return instructions

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
