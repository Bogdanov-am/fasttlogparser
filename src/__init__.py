"""Lightweight Python wrapper for the compiled fasttlogparser extension.

This module re-exports the compiled `parseTLog` function and provides a
small documented Python wrapper with type annotations for IDEs and linters.
"""

import numpy
from .fasttlogparser import parseTLog as parse


# pylint: disable=invalid-name
def parseTLog(
    path: str,
    ids: list[tuple[int, int]] | None = None,
    whitelist: list[str] | None = None,
    blacklist: list[str] | None = None,
    remap_field: dict[str, str] | None = None,
) -> tuple[dict[str, dict[str, numpy.ndarray]], dict[int, set[int]]]:
    """Parse a MAVLink .tlog file and return message series and observed IDs.

    This is a thin wrapper around the compiled `parseTLog` extension which
    provides a more convenient import path for Python callers.

    Args:
        path: Path to the .tlog file.
        ids: Optional list of (system, component) id tuples to filter messages.
        whitelist: Optional list of message names to include.
        blacklist: Optional list of message names to exclude.
        remap_field: Optional mapping to rename fields in output.

    Returns:
        A pair (messages, msg_ids) where `messages` is a mapping from message
        name to field arrays and `msg_ids` maps system id to observed component
        ids.
    """
    return parse(path, ids, whitelist, blacklist, remap_field)
