"""Pytest configuration file."""

import pytest
from ragger.backend import BackendInterface
from ragger.conftest import configuration
from ragger.firmware import Firmware
from ragger.navigator import Navigator
from utils.client import TezosClient
from utils.navigator import TezosNavigator
from common import DEFAULT_SEED

configuration.OPTIONAL.CUSTOM_SEED = DEFAULT_SEED

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )

@pytest.fixture(scope="function")
def client(backend: BackendInterface):
    """Get a tezos client."""
    return TezosClient(backend)

@pytest.fixture(scope="function")
def tezos_navigator(backend: BackendInterface,
                    client: TezosClient,
                    firmware: Firmware,
                    navigator: Navigator):
    """Get a tezos navigator."""
    return TezosNavigator(backend, firmware, client, navigator)
