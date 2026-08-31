#########################
##                     ##
##    STRUCTS & FXNS   ##
##                     ##    
#########################

# run: "python -m doctest structs.py"

from test import test_n


def run_structs_tests():
    """
    >>> run_structs_tests()
    structs test: DONE.
    """
    TESTS = [
        test_n(
            "structs", 1, "Struct Field Access and Mutation",
            "struct Point {\n"
            "    int x;\n"
            "    int y;\n"
            "};\n"
            "\n"
            "fxn run() {\n"
            "    Point p;\n"
            "    p.x = 10;\n"
            "    p.y = 20;\n"
            "    println(\"Point: \", p.x, \", \", p.y);\n"
            "}\n",
            "Point: 10, 20"
        ),

        test_n(
            "structs", 2, "Struct Methods with self Parameter",
            "struct Counter {\n"
            "    int value;\n"
            "};\n"
            "\n"
            "fxns Counter {\n"
            "    init(self, int start) {\n"
            "        self.value = start;\n"
            "    }\n"
            "    increment(self) {\n"
            "        self.value++;\n"
            "    }\n"
            "    get(self) -> int {\n"
            "        return self.value;\n"
            "    }\n"
            "}\n"
            "\n"
            "fxn run() {\n"
            "    Counter c;\n"
            "    c.init(5);\n"
            "    c.increment();\n"
            "    c.increment();\n"
            "    println(\"Counter: \", c.get());\n"
            "}\n",
            "Counter: 7"
        ),

        test_n(
            "structs", 3, "Static Methods and Namespaces",
            "struct Math {\n"
            "    int dummy;\n"
            "};\n"
            "\n"
            "fxns Math {\n"
            "    square(int n) -> int {\n"
            "        return n * n;\n"
            "    }\n"
            "    cube(int n) -> int {\n"
            "        return n * n * n;\n"
            "    }\n"
            "}\n"
            "\n"
            "fxn run() {\n"
            "    println(\"Square 6: \", Math.square(6));\n"
            "    println(\"Cube 3: \", Math.cube(3));\n"
            "}\n",
            "Square 6: 36\nCube 3: 27"
        ),

        test_n(
            "structs", 4, "Struct Arithmetic Combination (caleb.apl pattern)",
            "struct Calculator {\n"
            "    int a;\n"
            "    int b;\n"
            "};\n"
            "\n"
            "fxns Calculator {\n"
            "    set(self, int na, int nb) {\n"
            "        self.a = na;\n"
            "        self.b = nb;\n"
            "    }\n"
            "    add(self) -> int {\n"
            "        return self.a + self.b;\n"
            "    }\n"
            "    multiply(self) -> int {\n"
            "        return self.a * self.b;\n"
            "    }\n"
            "}\n"
            "\n"
            "fxn run() {\n"
            "    Calculator calc;\n"
            "    calc.set(40, 60);\n"
            "    println(\"Sum: \", calc.add());\n"
            "    println(\"Product: \", calc.multiply());\n"
            "}\n",
            "Sum: 100\nProduct: 2400"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("structs test: DONE.")
    else:
        print(f"structs test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_structs_tests()
