"""
RUN PRINTLN() TESTS

"""
import subprocess
from pathlib import Path
import os

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
        print(f"TEST-> {num} ({name}) PASSED")
        return 1
    else:
        print(f"TEST-> {num} ({name}) FAILED")
        print("Expected:", expected)
        print("Got:", result.stdout)
        if result.stderr:
            print("Stderr:", result.stderr)
        
        return 0

def run_all_tests():
    
    


    TESTS = [
    test_n(1,"String Literals", "fxn run() -> (void) { println(\"hello world\"); }", "hello world"),
    test_n(2,"Integers", "fxn run() -> (void) { println(5); }", "5"),
    test_n(3,"Variable", "fxn run() -> (void) { var name = \"Apollo\"; var year = 2026; println(name + \" was made in \" + year); }", 
    "Apollo was made in 2026"),

    test_n(4, "Sum", "fxn run() -> (void) { println(5 + 10); }", "15"),
    test_n(5, "Product", "fxn run() -> (void) { println(5 * 10); }", "50"),
    test_n(6, "Quotient", "fxn run() -> (void) { println(10 / 5); }", "2")
    
    ]


    pass_count = 0
    for test in TESTS:
        if  TESTS[test] == 1:
            pass_count += 1

    print(f"\nTESTING PRINTLN: DONE \n {pass_count}/{len(TESTS)} passed")

    
  

if __name__ == "__main__":
    run_all_tests()