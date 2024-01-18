"""Module of test helper functions."""

from typing import TypeVar, Callable, List, Union

from multiprocessing import Process, Queue

from ragger.navigator import NavInsID, NavIns

def run_simultaneously(processes: List[Process]) -> None:
    """Runs several processes simultaneously."""

    for process in processes:
        process.start()

    for process in processes:
        process.join()
        assert process.exitcode == 0, "Should have terminate successfully"

RESPONSE = TypeVar('RESPONSE')

def send_and_navigate(send: Callable[[], RESPONSE], navigate: Callable[[], None]) -> RESPONSE:
    """Sends a request and navigates before receiving a response."""

    def _send(result_queue: Queue) -> None:
        result_queue.put(send())

    result_queue: Queue = Queue()
    send_process = Process(target=_send, args=(result_queue,))
    navigate_process = Process(target=navigate)
    run_simultaneously([navigate_process, send_process])
    return result_queue.get()

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
