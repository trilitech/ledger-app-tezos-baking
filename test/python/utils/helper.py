"""Module of test helper functions."""

import git

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
