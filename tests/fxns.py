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
    )  
    ]

    for test in TESTS:
        TESTS[test]

    print(f"fxns test: DONE.")

    