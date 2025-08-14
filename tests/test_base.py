"""
Tests for the basic functionality of the fasttlogparser module.

These tests use a small flight log sample in `dev/flightlog.tlog` and a temporary
empty tlog to check behaviour on minimal input. The focus is on ensuring the
parser returns expected data structures and respects filtering options.
"""

import numpy
import pytest
import fasttlogparser

FILE = "dev/flightlog.tlog"


@pytest.fixture(name="tmp_tlog_file")
def setup_tmp_tlog_file(tmp_path):
    """
    Create and return a minimal (currently empty) tlog file path.

    The fixture writes an empty file to the temporary path. This ensures the
    parser can handle empty input without raising unexpected exceptions. In the
    future this fixture can be extended to write real MAVLink messages for
    more detailed tests.
    """
    tlog = tmp_path / "test.tlog"
    tlog.write_bytes(b"")  # empty file for now to make tests work in CI
    return tlog


def test_parse_basic():
    """
    Basic parsing should succeed and return the expected container types and
    field array types for a known message (ATTITUDE).
    """
    messages, msg_ids = fasttlogparser.parseTLog(FILE)
    assert isinstance(messages, dict)
    assert isinstance(msg_ids, dict)
    assert isinstance(messages["ATTITUDE"], dict)
    assert isinstance(messages["ATTITUDE"]["pitch"], numpy.ndarray)


def test_parse_with_ids():
    """
    Ensure parsing with specific (system,component) ID filters returns only
    messages matching those IDs and that msg_ids is populated accordingly.
    """
    messages, msg_ids = fasttlogparser.parseTLog(FILE, ids=[(5, 1)])
    assert len(messages["ATTITUDE"]["pitch"]) != 0
    assert len(messages["ATTITUDE"]["pitch"]) == len(messages["ATTITUDE"]["roll"])
    assert len(msg_ids) == 1

    messages, msg_ids = fasttlogparser.parseTLog(FILE, ids=[(1, 1)])
    assert len(messages) == 0
    assert len(msg_ids) == 1


def test_parse_with_whitelist():
    """
    Check that providing a whitelist of message names limits the parsed output
    to the allowed messages only.
    """
    messages, _ = fasttlogparser.parseTLog(FILE, whitelist=["GPS_RAW_INT"])
    assert "GPS_RAW_INT" in messages
    assert "ATTITUDE" not in messages


def test_parse_with_blacklist():
    """
    Check that providing a blacklist of message names excludes those messages
    from the parsed output.
    """
    messages, _ = fasttlogparser.parseTLog(FILE, blacklist=["ATTITUDE"])
    assert "GPS_RAW_INT" in messages
    assert "ATTITUDE" not in messages


def test_parse_with_remap():
    """
    Verify that field remapping (remap_field) renames fields in the parsed
    output as expected.
    """
    messages, _ = fasttlogparser.parseTLog(FILE, remap_field={"alt": "altitude"})
    assert "altitude" in messages["GPS_RAW_INT"]
    assert "alt" not in messages["GPS_RAW_INT"]


def test_invalid_path():
    """
    Passing a non-existent file path should raise an exception.
    """
    with pytest.raises(Exception):
        fasttlogparser.parseTLog("non_existent_file.tlog")


def test_empty_file(tmp_tlog_file):
    """
    Parsing an empty tlog file should return an empty messages mapping and not
    raise an exception.
    """
    messages, _ = fasttlogparser.parseTLog(str(tmp_tlog_file))
    assert len(messages) == 0
