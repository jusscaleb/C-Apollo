#########################
##                     ##
##     MEMORY TESTS    ##
##                     ##    
#########################

# run: "python -m doctest memory.py"

from test import test_n


def run_memory_tests():
    """
    >>> run_memory_tests()
    memory test: DONE.
    """
    TESTS = [
        test_n(
            "memory", 1, "Bucket 1 Static Segment Constants",
            "int APP_VERSION = 3;\n"
            "str APP_CODENAME = \"Apollo\";\n"
            "\n"
            "fxn run() {\n"
            "    println(\"Engine: \", APP_CODENAME, \" v\", APP_VERSION);\n"
            "}\n",
            "Engine: Apollo v3"
        ),

        test_n(
            "memory", 2, "Bucket 0 Stack Variables and Arrays",
            "fxn run() {\n"
            "    int local_counter = 100;\n"
            "    int[3] local_stack_arr = { 10, 20, 30 };\n"
            "    println(\"Counter: \", local_counter, \", Array[1]: \", local_stack_arr[1]);\n"
            "}\n",
            "Counter: 100, Array[1]: 20"
        ),

        test_n(
            "memory", 3, "Bucket 2 Loop Bookmark Memory Stability",
            "fxn run() {\n"
            "    var acc = 0;\n"
            "    for (var i = 0; i < 10; i++) {\n"
            "        var temp_str = \"iteration\";\n"
            "        acc += i;\n"
            "    }\n"
            "    println(\"Loop Accumulator: \", acc);\n"
            "}\n",
            "Loop Accumulator: 45"
        ),

        test_n(
            "memory", 4, "3+1 Bucket Multi-Region Integration",
            "int GLOBAL_MAX = 500;\n"
            "\n"
            "struct Node {\n"
            "    int val;\n"
            "};\n"
            "\n"
            "fxn run() {\n"
            "    Node n;\n"
            "    n.val = 250;\n"
            "    int[2] local_arr = { 50, 100 };\n"
            "    println(\"Max: \", GLOBAL_MAX, \", Node: \", n.val, \", Arr[0]: \", local_arr[0]);\n"
            "}\n",
            "Max: 500, Node: 250, Arr[0]: 50"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("memory test: DONE.")
    else:
        print(f"memory test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_memory_tests()
