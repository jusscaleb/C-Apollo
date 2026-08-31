#########################
##                     ##
##     ARRAY TESTS     ##
##                     ##    
#########################

# run: "python -m doctest arrays.py"

from test import test_n


def run_arrays_tests():
    """
    >>> run_arrays_tests()
    arrays test: DONE.
    """
    TESTS = [
        test_n(
            "arrays", 1, "Fixed Stack Array Element Access",
            "fxn run() {\n"
            "    int[5] arr = { 10, 20, 30, 40, 50 };\n"
            "    println(\"Element 0: \", arr[0]);\n"
            "    println(\"Element 4: \", arr[4]);\n"
            "}\n",
            "Element 0: 10\nElement 4: 50"
        ),

        test_n(
            "arrays", 2, "Fixed Stack Array Mutation",
            "fxn run() {\n"
            "    int[3] values = { 1, 2, 3 };\n"
            "    values[1] = 99;\n"
            "    println(\"Updated Element 1: \", values[1]);\n"
            "}\n",
            "Updated Element 1: 99"
        ),

        test_n(
            "arrays", 3, "Dynamic Arena Array Initialization",
            "fxn run() {\n"
            "    int[] dyn_list = { 100, 200, 300 };\n"
            "    println(\"Dyn Element 0: \", dyn_list[0]);\n"
            "    println(\"Dyn Element 2: \", dyn_list[2]);\n"
            "}\n",
            "Dyn Element 0: 100\nDyn Element 2: 300"
        ),

        test_n(
            "arrays", 4, "Array Iteration with For Loop",
            "fxn run() {\n"
            "    int[4] nums = { 5, 10, 15, 20 };\n"
            "    var total = 0;\n"
            "    for (var i = 0; i < 4; i++) {\n"
            "        total += nums[i];\n"
            "    }\n"
            "    println(\"Array Total: \", total);\n"
            "}\n",
            "Array Total: 50"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("arrays test: DONE.")
    else:
        print(f"arrays test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_arrays_tests()
