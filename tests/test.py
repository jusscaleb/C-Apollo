import subprocess
import shutil
import os
import sys
from pathlib import Path

RUN_SH = Path(__file__).parent.parent / "run.sh"
PROJECT_ROOT = Path(__file__).parent.parent
APOLLO_EXE = PROJECT_ROOT / "build" / "apollo.exe"


def test_n(test_type, num, name, code, expected):
    """
    Writes code and runs it to get the output and matches against expected output.
    Params:
    test_type (STRING): Category of the test being executed (folder name).
    num (INT): Used to differentiate and separate test files for review.
    name (STRING): The name of the test being executed.
    code (STRING): The code written in the .apl file to be tested.
    expected (STRING): Expected output to be compared to the actual output.
    """
    test_apl = Path(__file__).parent / f"temp/{test_type}/test{num}.apl"
    test_apl.parent.mkdir(parents=True, exist_ok=True)
    with open(test_apl, "w", encoding="utf-8") as f:
        f.write(code)

    # RUN COMPILER VIA RUN.SH OR DIRECT EXECUTABLE
    try:
        bash_cmd = shutil.which("bash") or shutil.which("sh")
        if bash_cmd and RUN_SH.exists():
            result = subprocess.run(
                [bash_cmd, str(RUN_SH), str(test_apl)],
                capture_output=True,
                text=True,
                cwd=str(PROJECT_ROOT)
            )
        elif APOLLO_EXE.exists():
            result = subprocess.run(
                [str(APOLLO_EXE), str(test_apl)],
                capture_output=True,
                text=True,
                cwd=str(PROJECT_ROOT)
            )
        else:
            result = subprocess.run(
                ["./build/apollo.exe", str(test_apl)],
                capture_output=True,
                text=True,
                cwd=str(PROJECT_ROOT),
                shell=True
            )
    except Exception as e:
        print(f"Error running compiler on test {num} ({name}): {e}")
        return 0

    stdout = result.stdout or ""
    if expected not in stdout:
        print(f"\n[FAILED] Test {num}: {name} ({test_type})")
        print(f"  Expected to contain: {expected!r}")
        print(f"  Actual Stdout:       {stdout!r}")
        if result.stderr:
            print(f"  Actual Stderr:       {result.stderr!r}")
        return 0

    return 1


if __name__ == '__main__':
    print("==================================================")
    print("      APOLLO v3.0.0-unstable-preview TEST SUITE   ")
    print("==================================================")

    # Import test modules
    try:
        from println import run_println_tests
        from conditionals import run_conditionals_tests
        from variable import run_var_tests
        from fxns import run_fxns_tests
        from structs import run_structs_tests
        from pointers import run_pointers_tests
        from arrays import run_arrays_tests
        from memory import run_memory_tests

        suites = [
            ("Println Suite", run_println_tests),
            ("Variables Suite", run_var_tests),
            ("Conditionals Suite", run_conditionals_tests),
            ("Functions Suite", run_fxns_tests),
            ("Structs & Methods Suite", run_structs_tests),
            ("Pointers Suite", run_pointers_tests),
            ("Arrays Suite", run_arrays_tests),
            ("Memory Architecture Suite", run_memory_tests)
        ]

        total_suites = len(suites)
        passed_suites = 0

        for name, suite_fn in suites:
            print(f"\n--- Running {name} ---")
            try:
                suite_fn()
                passed_suites += 1
            except Exception as ex:
                print(f"[ERROR] Suite '{name}' encountered an error: {ex}")

        print("\n==================================================")
        print(f"  SUMMARY: {passed_suites}/{total_suites} Suites Passed Successfully.")
        print("==================================================")
    except ImportError as e:
        print(f"Import error during test runner setup: {e}")