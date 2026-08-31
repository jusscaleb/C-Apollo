#########################
##                     ##
##    POINTER TESTS    ##
##                     ##    
#########################

# run: "python -m doctest pointers.py"

from test import test_n


def run_pointers_tests():
    """
    >>> run_pointers_tests()
    pointers test: DONE.
    """
    TESTS = [
        test_n(
            "pointers", 1, "Address-of and Dereferencing",
            "fxn run() {\n"
            "    int val = 42;\n"
            "    int* ptr = &val;\n"
            "    println(\"Original: \", val);\n"
            "    println(\"Via Pointer: \", *ptr);\n"
            "}\n",
            "Original: 42\nVia Pointer: 42"
        ),

        test_n(
            "pointers", 2, "Pointer Mutation",
            "fxn run() {\n"
            "    int score = 50;\n"
            "    int* ptr = &score;\n"
            "    *ptr = 100;\n"
            "    println(\"Mutated Score: \", score);\n"
            "}\n",
            "Mutated Score: 100"
        ),

        test_n(
            "pointers", 3, "Float Pointer Dereferencing",
            "fxn run() {\n"
            "    float pi = 3.14159;\n"
            "    float* fptr = &pi;\n"
            "    println(\"Float Pointer: \", *fptr);\n"
            "}\n",
            "Float Pointer: 3.14159"
        ),

        test_n(
            "pointers", 4, "Passing Pointer into Function",
            "fxn double_in_place(int* p) {\n"
            "    *p = *p * 2;\n"
            "}\n"
            "fxn run() {\n"
            "    int number = 21;\n"
            "    double_in_place(&number);\n"
            "    println(\"Doubled: \", number);\n"
            "}\n",
            "Doubled: 42"
        )
    ]

    passed = sum(1 for res in TESTS if res == 1)
    if passed == len(TESTS):
        print("pointers test: DONE.")
    else:
        print(f"pointers test: {passed}/{len(TESTS)} passed.")


if __name__ == '__main__':
    run_pointers_tests()
