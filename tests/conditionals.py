#########################
##                     ##
##    CONDITIONALS     ##
##                     ##    
#########################

# run: "python -m doctest conditionals.py"

from test import test_n


def run_conditionals_tests():
    """
    >>> run_conditionals_tests()
    conditionals test: DONE.
    """
    TESTS = [
        test_n(
            "conditionals", 1, "Basic if Statement",
            "fxn run() {\n"
            "    var score = 100;\n"
            "    if (score == 100) {\n"
            "        println(\"Perfect Score\");\n"
            "    }\n"
            "}\n",
            "Perfect Score"
        ),

        test_n(
            "conditionals", 2, "if -> else Statement",
            "fxn run() {\n"
            "    var score = 45;\n"
            "    if (score >= 50) {\n"
            "        println(\"Passed\");\n"
            "    } else {\n"
            "        println(\"Failed\");\n"
            "    }\n"
            "}\n",
            "Failed"
        ),

        test_n(
            "conditionals", 3, "if -> elif -> else Multi-Branch",
            "fxn run() {\n"
            "    var grade = 85;\n"
            "    if (grade >= 90) {\n"
            "        println(\"Grade A\");\n"
            "    } elif (grade >= 80) {\n"
            "        println(\"Grade B\");\n"
            "    } else {\n"
            "        println(\"Grade C\");\n"
            "    }\n"
            "}\n",
            "Grade B"
        ),

        test_n(
            "conditionals", 4, "Logical AND Condition",
            "fxn run() {\n"
            "    var age = 25;\n"
            "    var has_id = true;\n"
            "    if (age >= 18 and has_id) {\n"
            "        println(\"Access Granted\");\n"
            "    }\n"
            "}\n",
            "Access Granted"
        ),

        test_n(
            "conditionals", 5, "Logical OR Condition",
            "fxn run() {\n"
            "    var is_admin = false;\n"
            "    var is_owner = true;\n"
            "    if (is_admin or is_owner) {\n"
            "        println(\"Authorized\");\n"
            "    }\n"
            "}\n",
            "Authorized"
        ),

        test_n(
            "conditionals", 6, "While Loop Execution",
            "fxn run() {\n"
            "    var counter = 3;\n"
            "    while (counter > 0) {\n"
            "        println(\"Count: \", counter);\n"
            "        counter--;\n"
            "    }\n"
            "}\n",
            "Count: 3\nCount: 2\nCount: 1"
        ),

        test_n(
            "conditionals", 7, "For Loop Accumulator",
            "fxn run() {\n"
            "    var sum = 0;\n"
            "    for (var i = 1; i <= 5; i++) {\n"
            "        sum += i;\n"
            "    }\n"
            "    println(\"Sum: \", sum);\n"
            "}\n",
            "Sum: 15"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("conditionals test: DONE.")
    else:
        print(f"conditionals test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_conditionals_tests()