import os
import time
import subprocess
import shutil
import json
import math
from datetime import datetime
from pathlib import Path

# Paths
PERFORMANCE_DIR = Path(__file__).parent
PROJECT_ROOT = PERFORMANCE_DIR.parent.parent
APL_EXE = PROJECT_ROOT / "apl.exe"
TEMP_DIR = PERFORMANCE_DIR / "temp"
HISTORY_FILE = PERFORMANCE_DIR / "performance_history.json"
LOG_FILE = PERFORMANCE_DIR / "performance_log.txt"

def setup():
    if not TEMP_DIR.exists():
        TEMP_DIR.mkdir(parents=True)
    
    if not APL_EXE.exists():
        print(f"Error: Could not find compiler at {APL_EXE}")
        print("Please compile Apollo first using GCC.")
        exit(1)

def cleanup():
    if TEMP_DIR.exists():
        try:
            shutil.rmtree(TEMP_DIR)
        except PermissionError:
            print("Warning: Could not delete temp directory due to a PermissionError. A process may still be using it.")

def calculate_stats(values):
    if not values:
        return 0.0, 0.0
    avg = sum(values) / len(values)
    if len(values) > 1:
        variance = sum((x - avg) ** 2 for x in values) / (len(values) - 1)
        std_dev = math.sqrt(variance)
    else:
        std_dev = 0.0
    pct_error = (std_dev / avg * 100.0) if avg > 0 else 0.0
    return avg, pct_error

def run_single_iteration(name, source_code, iteration):
    source_file = TEMP_DIR / f"{name}.apl"
    with open(source_file, "w", encoding="utf-8") as f:
        f.write(source_code)
        
    start_compile = time.perf_counter()
    compile_process = subprocess.run(
        [str(APL_EXE), str(source_file), str(PROJECT_ROOT)],
        capture_output=True,
        text=True,
        cwd=str(TEMP_DIR)
    )
    end_compile = time.perf_counter()
    compile_time = end_compile - start_compile
    
    if compile_process.returncode != 0:
        print(f"  [Run {iteration}] Compilation returned non-zero exit code.")
        
    program_exe = TEMP_DIR / "temp" / "program.exe"
    if not program_exe.exists():
        print(f"  [Run {iteration}] Error: Compiled executable not found at {program_exe}")
        return compile_time, 0.0, compile_time
        
    start_run = time.perf_counter()
    run_process = subprocess.run(
        [str(program_exe)],
        capture_output=True,
        text=True,
        cwd=str(TEMP_DIR)
    )
    end_run = time.perf_counter()
    run_time = end_run - start_run
    
    if run_process.returncode != 0:
        print(f"  [Run {iteration}] Execution returned non-zero exit code.")
        
    total_time = compile_time + run_time
    return compile_time, run_time, total_time

def run_benchmark(name, source_code, repeats=3):
    print(f"--- Running Benchmark: {name} ({repeats} Runs) ---")
    
    compile_times = []
    run_times = []
    total_times = []
    
    for i in range(1, repeats + 1):
        ct, rt, tt = run_single_iteration(name, source_code, i)
        compile_times.append(ct)
        run_times.append(rt)
        total_times.append(tt)
        print(f"  Run {i}: Compile: {ct:.4f}s | Execute: {rt:.4f}s | Total: {tt:.4f}s")
        
    avg_ct, ct_err = calculate_stats(compile_times)
    avg_rt, rt_err = calculate_stats(run_times)
    avg_tt, tt_err = calculate_stats(total_times)
    
    print(f"  --> Average Compile Time: {avg_ct:.4f}s (+/-{ct_err:.2f}% Error)")
    print(f"  --> Average Execute Time: {avg_rt:.4f}s (+/-{rt_err:.2f}% Error)")
    print(f"  --> Average Total Time:   {avg_tt:.4f}s (+/-{tt_err:.2f}% Error)\n")
    
    return {
        "compile_time": avg_ct,
        "compile_err": ct_err,
        "run_time": avg_rt,
        "run_err": rt_err,
        "total_time": avg_tt,
        "total_err": tt_err,
        "repeats": repeats
    }

def update_performance_log(current_run_results):
    history = []
    if HISTORY_FILE.exists():
        with open(HISTORY_FILE, "r", encoding="utf-8") as f:
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
        
    with open(HISTORY_FILE, "w", encoding="utf-8") as f:
        json.dump(history, f, indent=4)
        
    with open(LOG_FILE, "w", encoding="utf-8") as f:
        f.write("Apollo Performance Log\n")
        f.write("========================\n\n")
        
        for record in history:
            f.write(f"Date and Time: {record['timestamp']}\n")
            f.write(f"Overall Total Speed: {record['total_speed']:.4f} seconds\n")
            for b_name, b_res in record['results'].items():
                f.write(f"  - {b_name}:\n")
                if 'compile_err' in b_res:
                    f.write(f"      Compile: {b_res['compile_time']:.4f}s (+/-{b_res['compile_err']:.2f}% Error)\n")
                    f.write(f"      Execute: {b_res['run_time']:.4f}s (+/-{b_res['run_err']:.2f}% Error)\n")
                    f.write(f"      Total:   {b_res['total_time']:.4f}s (+/-{b_res['total_err']:.2f}% Error)\n")
                else:
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
    print("Running 3 repetitions per benchmark with Percentage Error calculation.")
    print("=" * 60)
    setup()
    
    current_results = {}
    
    try:
        small_code = "fxn run()->(void){\n    int x = 42;\n    println(x);\n}\n"
        current_results["small_program"] = run_benchmark("small_program", small_code, repeats=3)
        
        large_compile_code = "fxn run()->(void){\n"
        for i in range(1000):
            large_compile_code += f"    int x{i} = {i};\n"
        large_compile_code += "}\n"
        current_results["large_compilation"] = run_benchmark("large_compilation", large_compile_code, repeats=3)

        for_loop = "fxn run()->(void){\n   int x; for(int i = 0; i < 1000000; i++){ x += i;} println(x);\n}"
        current_results["for_loop"] = run_benchmark("for_loop", for_loop, repeats=3)

        var_decl_with_dt = "fxn run()->(void){\n   int x = 5; int z; z = x + 10; println(z);\n}"
        current_results["var_decl_with_dt"] = run_benchmark("var_decl_with_dt", var_decl_with_dt, repeats=3)

        var_decl_no_dt = "fxn run()->(void){\n   var x = 5; var z; z = x + 10; println(z);\n}"
        current_results["var_decl_no_dt"] = run_benchmark("var_decl_no_dt", var_decl_no_dt, repeats=3)

        update_performance_log(current_results)
        
    finally:
        cleanup()
        print(f"Benchmarks completed. See {LOG_FILE.name} for detailed history and statistics.")
