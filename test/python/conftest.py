"""Pytest configuration file."""

import pytest
from ragger.backend import BackendInterface
from utils.client import TezosClient

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )

@pytest.fixture(scope="function")
def client(backend: BackendInterface):
    """Get a tezos client."""
    return TezosClient(backend)
