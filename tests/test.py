import subprocess, shutil
from pathlib import Path


RUN_SH = Path(__file__).parent.parent / "run.sh"
PROJECT_ROOT = Path(__file__).parent.parent


def test_n(type, num ,name, code, expected):
    """
    Writes code and runs it to get the output and matches against expected output.
    Params:
    type (STRING): Category of the test being executed. (Acts as the folder name).
    num (INT): Used to differentiate and separate test files for review.
    name (STRING): The name of the test being executed.
    code (STRING): The code written in the .apl file to be tested.
    expected (STRING): Expected output to be compared to the actual output.
    """

    try:
        TEST_APL = Path(__file__).parent / f"temp/{type}/test{num}.apl"
        TEST_APL.parent.mkdir(parents=True, exist_ok=True)
        with open(TEST_APL, "w", encoding="utf-8") as f:
            f.write(code)
    
    except FileNotFoundError:
        print(f"Please run execute this through diagnosis.sh with the command: 'bash diagnosis.sh {type}'.")
        print("Ensure you have gitbash installed; run through gitbash terminal.")
        
    # RUN COMPILER VIA RUN.SH
    try:
        bash_cmd = shutil.which("bash") or shutil.which("sh") or "bash"
        result = subprocess.run([bash_cmd, str(RUN_SH), str(TEST_APL)], 
                                capture_output=True, text=True, cwd=str(PROJECT_ROOT))
    except Exception as e:
        print(f"Error running compiler: {e}")
        return 0
    
    if expected not in result.stdout:
        print(f"TEST-> {num} ({name}) FAILED")
        print("Expected:", expected)
        print("Got:", result.stdout)
        return 0
    
    return 1
    
        
       