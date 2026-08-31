#########################
##                     ##
##    PRINTLN() TEST   ##
##                     ##    
#########################

# run: "python -m doctest println.py"

from test import test_n


def run_println_tests():
    """
    >>> run_println_tests()
    println test: DONE.
    """
    TESTS = [
        test_n(
            "println", 1, "String Literals",
            "fxn run() {\n"
            "    println(\"hello world\");\n"
            "}\n",
            "hello world"
        ),

        test_n(
            "println", 2, "Integer Literals",
            "fxn run() {\n"
            "    println(42);\n"
            "}\n",
            "42"
        ),

        test_n(
            "println", 3, "Float Literals",
            "fxn run() {\n"
            "    println(3.14159);\n"
            "}\n",
            "3.14159"
        ),

        test_n(
            "println", 4, "Boolean Literals",
            "fxn run() {\n"
            "    println(true);\n"
            "    println(false);\n"
            "}\n",
            "true\nfalse"
        ),

        test_n(
            "println", 5, "Variadic Multiple Arguments",
            "fxn run() {\n"
            "    var name = \"Apollo\";\n"
            "    var year = 2026;\n"
            "    println(name, \" was created in \", year);\n"
            "}\n",
            "Apollo was created in 2026"
        ),

        test_n(
            "println", 6, "Arithmetic Expression Inside Println",
            "fxn run() {\n"
            "    println(15 + 27 * 2);\n"
            "}\n",
            "69"
        ),

        test_n(
            "println", 7, "Character Literal",
            "fxn run() {\n"
            "    println('A');\n"
            "}\n",
            "A"
        ),

        test_n(
            "println", 8, "Escape Characters",
            "fxn run() {\n"
            "    println(\"Line1\\nLine2\\tTabbed\");\n"
            "}\n",
            "Line1\nLine2\tTabbed"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("println test: DONE.")
    else:
        print(f"println test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_println_tests()