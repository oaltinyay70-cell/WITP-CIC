"""
Purpose: Script to bundle the application into a standalone .exe using PyInstaller.
Version: 0.01
Author: Assistant / Antigravity
Dependencies: PyInstaller

This script handles the compilation of the Python source into an executable.
"""

import subprocess
import sys
from pathlib import Path

def build_executable() -> None:
    """
    Runs PyInstaller to build the .exe file.

    Args:
        None

    Returns:
        None

    Raises:
        subprocess.CalledProcessError: If the PyInstaller process fails.

    Example:
        >>> build_executable()
    """
    project_root = Path(__file__).resolve().parent.parent
    main_script = project_root / "src" / "witp_ae_helper" / "__main__.py"
    
    # We use --onedir or --onefile. The user requested "an exe file", usually implying --onefile.
    # --windowed removes the console (useful once GUI is ready).
    command = [
        sys.executable, "-m", "PyInstaller",
        "--name", "WITPAE_Helper",
        "--onefile",
        "--noconfirm",
        str(main_script)
    ]
    
    print("Running PyInstaller...")
    # Execute the PyInstaller build
    subprocess.run(command, check=True, cwd=project_root)
    print("Build complete. The .exe file can be found in the 'dist' folder.")

if __name__ == "__main__":
    build_executable()
