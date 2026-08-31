#########################
##                     ##
##    VARIABLE TESTS   ##
##                     ##    
#########################

# run: "python -m doctest variable.py"

from test import test_n


def run_var_tests():
    """
    >>> run_var_tests()
    Variable test: DONE.
    """
    TESTS = [
        test_n(
            "variable", 1, "Variable Naming Formats",
            "fxn run() {\n"
            "    var camelCase = 10;\n"
            "    var snake_case = 20;\n"
            "    var PascalCase = 30;\n"
            "    println(camelCase + snake_case + PascalCase);\n"
            "}\n",
            "60"
        ),

        test_n(
            "variable", 2, "Explicit Types Assignment",
            "fxn run() {\n"
            "    int a = 100;\n"
            "    float b = 2.5;\n"
            "    bool c = true;\n"
            "    char d = 'Z';\n"
            "    str e = \"Apollo\";\n"
            "    println(a, \" \", b, \" \", c, \" \", d, \" \", e);\n"
            "}\n",
            "100 2.500000 true Z Apollo"
        ),

        test_n(
            "variable", 3, "Compound In-Place Operators",
            "fxn run() {\n"
            "    var x = 10;\n"
            "    x += 5;\n"
            "    x -= 3;\n"
            "    x *= 2;\n"
            "    x /= 4;\n"
            "    println(\"Result: \", x);\n"
            "}\n",
            "Result: 6"
        ),

        test_n(
            "variable", 4, "Increment and Decrement",
            "fxn run() {\n"
            "    var count = 5;\n"
            "    count++;\n"
            "    count++;\n"
            "    count--;\n"
            "    println(\"Count: \", count);\n"
            "}\n",
            "Count: 6"
        ),

        test_n(
            "variable", 5, "Compile-Time Static Constants",
            "int MAX_LIMIT = 500;\n"
            "str SYSTEM_ID = \"APOLLO-01\";\n"
            "\n"
            "fxn run() {\n"
            "    println(SYSTEM_ID, \" Limit: \", MAX_LIMIT);\n"
            "}\n",
            "APOLLO-01 Limit: 500"
        ),

        test_n(
            "variable", 6, "Reassignment Across Multiple Steps",
            "fxn run() {\n"
            "    var score = 10;\n"
            "    score = score + 15;\n"
            "    score = score * 2;\n"
            "    println(\"Final Score: \", score);\n"
            "}\n",
            "Final Score: 50"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("Variable test: DONE.")
    else:
        print(f"Variable test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_var_tests()