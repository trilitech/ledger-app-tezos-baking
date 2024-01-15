from typing import List

from ragger.navigator import NavInsID, NavIns

def get_nano_review_instructions(num_screen_skip) -> List[NavIns]:
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions

def get_stax_review_instructions(num_screen_skip) -> List[NavIns]:
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)] * num_screen_skip
    instructions.append(NavIns(NavInsID.USE_CASE_CHOICE_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_DISMISS))
    return instructions

def get_stax_address_instructions() -> List[NavIns]:
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)]
    instructions.append(NavIns(NavInsID.TOUCH, (112, 251)))
    instructions.append(NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR))
    instructions.append(NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_DISMISS))
    return instructions
