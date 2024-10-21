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

"""Pytest configuration file."""

from functools import wraps
import pytest

from ragger.backend import BackendInterface
from ragger.conftest import configuration
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from utils.account import SigScheme
from utils.client import TezosClient
from utils.navigator import TezosNavigator
from common import DEFAULT_SEED

configuration.OPTIONAL.CUSTOM_SEED = DEFAULT_SEED

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )

@pytest.fixture(scope="function")
def client(backend: BackendInterface) -> TezosClient:
    """Get a tezos client."""
    return TezosClient(backend)

@pytest.fixture(scope="function")
def tezos_navigator(
        backend: BackendInterface,
        firmware: Firmware,
        client: TezosClient,
        navigator: Navigator,
        golden_run: bool,
        test_name: str) -> TezosNavigator:
    """Get a tezos navigator."""
    return TezosNavigator(backend, firmware, client, navigator, golden_run, test_name)

def skip_nanos_bls(func):
    """Allows to skip tests with BLS `account` on NanoS device"""
    @wraps(func)
    def wrap(*args, **kwargs):
        account = kwargs.get("account", None)
        firmware = kwargs.get("firmware", None)
        if firmware is not None \
           and firmware.name == "nanos" \
           and account is not None \
           and account.sig_scheme == SigScheme.BLS:
            pytest.skip("NanoS does not support BLS")
        return func(*args, **kwargs)
    return wrap
