import os
import time
import subprocess
import shutil
import json
from datetime import datetime
from pathlib import Path

# Paths
PERFORMANCE_DIR = Path(__file__).parent
PROJECT_ROOT = PERFORMANCE_DIR.parent.parent
RUN_SH = PROJECT_ROOT / "run.sh"
TEMP_DIR = PERFORMANCE_DIR / "temp"
HISTORY_FILE = PERFORMANCE_DIR / "performance_history.json"
LOG_FILE = PERFORMANCE_DIR / "performance_log.txt"

def setup():
    if not TEMP_DIR.exists():
        TEMP_DIR.mkdir(parents=True)
    
    if not RUN_SH.exists():
        print(f"Error: Could not find run script at {RUN_SH}")
        exit(1)

    print("Pre-building Apollo compiler...")
    bash_cmd = shutil.which("bash") or shutil.which("sh") or "bash"
    subprocess.run([bash_cmd, str(RUN_SH), "caleb.apl"], capture_output=True, cwd=str(PROJECT_ROOT))
    print("Pre-build done...")

def cleanup():
    if TEMP_DIR.exists():
        try:
            shutil.rmtree(TEMP_DIR)
        except PermissionError:
            print("Warning: Could not delete temp directory due to a PermissionError. A process may still be using it.")

def run_benchmark(name, source_code):
    print(f"--- Running Benchmark: {name} ---")
    
    source_file = TEMP_DIR / f"{name}.apl"
    with open(source_file, "w") as f:
        f.write(source_code)
        
    start_compile = time.perf_counter()
    bash_cmd = shutil.which("bash") or shutil.which("sh") or "bash"
    compile_process = subprocess.run(
        [bash_cmd, str(RUN_SH), str(source_file)],
        capture_output=True,
        text=True,
        cwd=str(PROJECT_ROOT)
    )
    end_compile = time.perf_counter()
    compile_time = end_compile - start_compile
    
    if compile_process.returncode != 0:
        print(f"Compilation returned non-zero code, but continuing...")
        
    print(f"Compilation Time: {compile_time:.4f} seconds")
    
    program_exe = PROJECT_ROOT / "temp" / "program.exe"
    if not program_exe.exists():
        program_exe = TEMP_DIR / "temp" / "program.exe"
    
    if not program_exe.exists():
        print(f"Error: Compiled executable not found at {program_exe}")
        return 0, 0, 0
        
    start_run = time.perf_counter()
    run_process = subprocess.run(
        [str(program_exe)],
        capture_output=True,
        text=True,
        cwd=str(PROJECT_ROOT)
    )
    end_run = time.perf_counter()
    run_time = end_run - start_run
    
    if run_process.returncode != 0:
        print(f"Execution returned non-zero code, but continuing...")
        
    print(f"Execution Time:   {run_time:.4f} seconds")
    total_time = compile_time + run_time
    print(f"Total Time:       {total_time:.4f} seconds\n")
    
    return compile_time, run_time, total_time

def update_performance_log(current_run_results):
    history = []
    if HISTORY_FILE.exists():
        with open(HISTORY_FILE, "r") as f:
            try:
                history = json.load(f)
            except json.JSONDecodeError:
                history = []
                
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Calculate current total speed
    current_total_speed = sum(b['total_time'] for b in current_run_results.values())
    
    # Check if the set of benchmarks has changed (new benchmark added or removed)
    if history:
        last_run_benchmarks = set(history[-1]['results'].keys())
        current_benchmarks = set(current_run_results.keys())
        if last_run_benchmarks != current_benchmarks:
            print("Notice: Benchmark suite changed. Resetting performance history.")
            history = []
    
    run_record = {
        "timestamp": now_str,
        "total_speed": current_total_speed,
        "results": current_run_results
    }
    
    history.append(run_record)
    
    # Keep at most last 20
    if len(history) > 20:
        history = history[-20:]
        
    with open(HISTORY_FILE, "w") as f:
        json.dump(history, f, indent=4)
        
    with open(LOG_FILE, "w") as f:
        f.write("Apollo Performance Log\n")
        f.write("========================\n\n")
        
        for record in history:
            f.write(f"Date and Time: {record['timestamp']}\n")
            f.write(f"Overall Total Speed: {record['total_speed']:.4f} seconds\n")
            for b_name, b_res in record['results'].items():
                f.write(f"  - {b_name}:\n")
                f.write(f"      Compile: {b_res['compile_time']:.4f}s\n")
                f.write(f"      Execute: {b_res['run_time']:.4f}s\n")
                f.write(f"      Total:   {b_res['total_time']:.4f}s\n")
            f.write("-" * 40 + "\n")
            
        f.write("\nSummary Statistics\n")
        f.write("========================\n")
        
        total_speeds = [r['total_speed'] for r in history]
        avg_speed = sum(total_speeds) / len(total_speeds)
        f.write(f"Average Speed (last {len(history)} runs): {avg_speed:.4f} seconds\n")
        
        if len(history) > 1:
            last_speed = history[-2]['total_speed']
            current_speed = history[-1]['total_speed']
            
            if last_speed > 0:
                diff = current_speed - last_speed
                pct_diff = (diff / last_speed) * 100
                if diff > 0:
                    f.write(f"Percentage Difference: +{pct_diff:.2f}% (Slower than last run)\n")
                else:
                    f.write(f"Percentage Difference: {pct_diff:.2f}% (Faster than last run)\n")
            else:
                f.write("Percentage Difference: N/A\n")
        else:
            f.write("Percentage Difference: N/A (Need at least 2 runs to compare)\n")

if __name__ == "__main__":

    print("Apollo Performance Benchmark\n")
    print("Note: there is a 10% error margin.")
    print("=" * 30)
    setup()
    
    current_results = {}
    
    try:
        small_code = "fxn run()->(void){\n    int x = 42;\n    println(x);\n}\n"
        ct, rt, tt = run_benchmark("small_program", small_code)
        current_results["small_program"] = {"compile_time": ct, "run_time": rt, "total_time": tt}
        
        large_compile_code = "fxn run()->(void){\n"
        for i in range(1000):
            large_compile_code += f"    int x{i} = {i};\n"
        large_compile_code += "}\n"
        ct, rt, tt = run_benchmark("large_compilation", large_compile_code)
        current_results["large_compilation"] = {"compile_time": ct, "run_time": rt, "total_time": tt}

        for_loop = "fxn run()->(void){\n   int x; for(int i = 0; i < 1000000; i++){ x += i;} println(x);\n}"
        ct, rt, tt = run_benchmark("for_loop", for_loop)
        current_results["for_loop"] = {"compile_time": ct, "run_time": rt, "total_time": tt}

        var_decl_with_dt = "fxn run()->(void){\n   int x = 5; int z; z = x + 10; println(z);\n}"
        ct, rt, tt = run_benchmark("var_decl_with_dt", var_decl_with_dt)
        current_results["var_decl_with_dt"] = {"compile_time": ct, "run_time": rt, "total_time": tt}

        var_decl_no_dt = "fxn run()->(void){\n   var x = 5; var z; z = x + 10; println(z);\n}"
        ct, rt, tt = run_benchmark("var_decl_no_dt", var_decl_no_dt)
        current_results["var_decl_no_dt"] = {"compile_time": ct, "run_time": rt, "total_time": tt}

        for_loop_continous_println = "fxn run()->(void){\n   int x; for(int i = 0; i < 1000000; i++){ x += i; println(x);} \n}"
        ct, rt, tt = run_benchmark("for_loop_continous_println", for_loop_continous_println)
        current_results["for_loop_continous_println"] = {"compile_time": ct, "run_time": rt, "total_time": tt}

        recursion = "fxn factorial(int n) -> int { if (n == 1) { return 1;}return n * factorial(n - 1);}fxn run() -> void {println(factorial(5));}"
        ct, rt, tt = run_benchmark("recursion", for_loop_continous_println)
        current_results["recursion"] = {"compile_time": ct, "run_time": rt, "total_time": tt}
        
        update_performance_log(current_results)
        
    finally:
        cleanup()
        print(f"Benchmarks completed. See {LOG_FILE.name} for detailed history and statistics.")
