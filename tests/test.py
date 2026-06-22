import subprocess
from pathlib import Path


APL = Path(__file__).parent.parent / "apl.exe"
PROJECT_ROOT = Path(__file__).parent.parent
TEST_APL = Path(__file__).parent / "test.apl"



def test_n(num ,name, code, expected):
        
    with open(TEST_APL, "w", encoding="utf-8") as f:
        f.write(code)

    # RUN COMPILER
    try:
        result = subprocess.run([str(APL), str(TEST_APL), str(PROJECT_ROOT)], 
                                capture_output=True, text=True, cwd=str(Path(__file__).parent))
    except Exception as e:
        print(f"Error running compiler: {e}")
        return
    
    if expected in result.stdout:
        return 1
    else:
        print(f"TEST-> {num} ({name}) FAILED")
        print("Expected:", expected)
        print("Got:", result.stdout)
        if result.stderr:
            print("Stderr:", result.stderr)
        
        return 0