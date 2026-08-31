#########################
##                     ##
##    FXN TEST         ##
##                     ##    
#########################

# run: "python -m doctest fxns.py"

from test import test_n


def run_fxns_tests():
    """
    >>> run_fxns_tests()
    fxns test: DONE.
    """
    TESTS = [
        test_n(
            "fxns", 1, "Void Function Call",
            "fxn greet() {\n"
            "    println(\"Hello from function\");\n"
            "}\n"
            "fxn run() {\n"
            "    greet();\n"
            "}\n",
            "Hello from function"
        ),

        test_n(
            "fxns", 2, "Function with Multiple Parameters",
            "fxn display_info(str name, int age) {\n"
            "    println(\"Name: \", name, \", Age: \", age);\n"
            "}\n"
            "fxn run() {\n"
            "    display_info(\"Caleb\", 24);\n"
            "}\n",
            "Name: Caleb, Age: 24"
        ),

        test_n(
            "fxns", 3, "Integer Return Type",
            "fxn multiply(int a, int b) -> int {\n"
            "    return a * b;\n"
            "}\n"
            "fxn run() {\n"
            "    var res = multiply(6, 7);\n"
            "    println(\"Product: \", res);\n"
            "}\n",
            "Product: 42"
        ),

        test_n(
            "fxns", 4, "Float Return Type",
            "fxn calculate_average(float x, float y) -> float {\n"
            "    return (x + y) / 2.0;\n"
            "}\n"
            "fxn run() {\n"
            "    println(\"Avg: \", calculate_average(10.0, 20.0));\n"
            "}\n",
            "15.0"
        ),

        test_n(
            "fxns", 5, "Boolean Return Type",
            "fxn is_even(int val) -> bool {\n"
            "    return (val % 2) == 0;\n"
            "}\n"
            "fxn run() {\n"
            "    println(\"4 is even: \", is_even(4));\n"
            "    println(\"7 is even: \", is_even(7));\n"
            "}\n",
            "4 is even: true\n7 is even: false"
        ),

        test_n(
            "fxns", 6, "Recursive Function",
            "fxn factorial(int n) -> int {\n"
            "    if (n <= 1) {\n"
            "        return 1;\n"
            "    }\n"
            "    return n * factorial(n - 1);\n"
            "}\n"
            "fxn run() {\n"
            "    println(\"Factorial 5: \", factorial(5));\n"
            "}\n",
            "Factorial 5: 120"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("fxns test: DONE.")
    else:
        print(f"fxns test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_fxns_tests()
