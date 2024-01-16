"""Pytest configuration file."""

import pytest
from ragger.backend import BackendInterface
from ragger.conftest import configuration
from utils.client import TezosClient

DEFAULT_SEED = " ".join(['zebra'] * 24)

configuration.OPTIONAL.CUSTOM_SEED = DEFAULT_SEED

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )

@pytest.fixture(scope="function")
def client(backend: BackendInterface):
    """Get a tezos client."""
    return TezosClient(backend)
