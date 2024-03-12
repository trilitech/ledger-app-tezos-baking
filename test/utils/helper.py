# Copyright 2024 Functori <contact@functori.com>
# Copyright 2024 Trilitech <contact@trili.tech>

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
