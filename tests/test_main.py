"""
Purpose: Test module for the main entry point.
Version: 0.01
Author: Assistant / Antigravity
Dependencies: pytest

This module tests the initialization and exit status of the main application entry point.
"""

from witp_ae_helper.__main__ import main

def test_main_exit_code() -> None:
    """
    Verifies that the main application returns a successful exit code (0).

    Args:
        None

    Returns:
        None

    Raises:
        AssertionError: If the main function does not return 0.

    Example:
        >>> test_main_exit_code()
    """
    # The main function currently just prints a message and returns 0.
    exit_code = main()
    assert exit_code == 0
