#########################
##                     ##
##    FXN TEST         ##
##                     ##    
#########################

#run "python -m doctest println.py"

from test import test_n



def run_fxns_tests():
    """
    >>> run_fxns_tests()
    fxns test: DONE.
    """
    TESTS = [

    test_n("fxns",
    1,
    "Fxn Call", 
    "### FXN CALL TEST ###\n"
    "fxn hello()->(void){ println(\"hello world\"); "
    "}"
    "fxn run() -> (void) { hello();" 
    "}"
    , "hello world"),

    test_n("fxns",2,"fxn_var", 
    "### VARIABLES ###\n"
    "fxn variables()->(void){ "
    "var number = 5;" 
    "println(number + 6);"
    "}"
    "fxn run() -> (void) {" \
    "var number = 6;"
    "println(number);" \
    "variables();"
    "}", "6\n11"),
    test_n("fxns", 3, "fxn_params",
     "###STRING PARAMETERS###\n\n"
     "fxn concatinate(str x, str y, int z){"
     "println(x +\" was made in \" + z + \" by \" + y);" 
     "}\n\n"
     
     "fxn run(){"
     "concatinate(\"Apollo\", \"Scxrpius\", 2026);"
     "}",
     "Apollo was made in 2026 by Scxrpius"
    ),

    test_n("fxns", 4, "int_return_with_params",
     "### INT RETURN WITH PARAMETERS ###\n"
     "fxn add(int x, int y)->(int){"
     "return x + y;"
     "}"
     "fxn run()->(void){"
     "println(add(4, 5));"
     "}",
     "9"
    ),

    test_n("fxns", 5, "float_return_with_params",
     "### FLOAT RETURN WITH PARAMETERS ###\n"
     "fxn add_float(float x, float y)->(float){"
     "return x + y;"
     "}"
     "fxn run()->(void){"
     "println(add_float(5.0, 7.0));"
     "}",
     "12.000000"
    ),

    test_n("fxns", 6, "string_return_with_params",
     "### STRING RETURN WITH PARAMETERS ###\n"
     "fxn label(str name, int year)->(str){"
     "return name + \" \" + year;"
     "}"
     "fxn run()->(void){"
     "println(label(\"Apollo\", 2026));"
     "}",
     "Apollo 2026"
    ),

    test_n("fxns", 7, "bool_return_with_params",
     "### BOOL RETURN WITH PARAMETERS ###\n"
     "fxn is_bigger(int left, int right)->(bool){"
     "return left > right;"
     "}"
     "fxn run()->(void){"
     "println(is_bigger(9, 4));"
     "}",
     "true"
    )
    ]

    for test in TESTS:
        TESTS[test]

    print(f"fxns test: DONE.")

    
