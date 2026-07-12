#########################
##                     ##
##    VARIABLE TESTS   ##
##                     ##    
#########################

from test import test_n 



def run_var_tests():
    """ 
    >>> run_var_tests()
    Variable test: DONE.
    
    """
    
    TESTS = [
        test_n("variable", 1, "Different Forms", "###DIFFERENT FORMS###\nfxn run()->(void){ var UPPERCASE = \"APOLLO\"; \n var lowecase = \"apollo\"; var snake_case = \"a_p_o_l_l_o\"; var turtleCase = \"apOLLO\"; println(\"SUCCESS\");}", "SUCCESS"),
        test_n("variable", 2, "Assignment", "###ASSIGNMENT###\nfxn run()->(void){ var x = 5; var y = 3; var z = 4 + 5; x = z + y; println(x);}", "12"),
        test_n("variable", 3, "Explicit Int Unassigned", "fxn run()->(void){ int x; println(x); }", "0\n"),
        test_n("variable", 4, "Explicit Float Unassigned", "fxn run()->(void){ float x; println(x); }", "0.000000\n"),
        test_n("variable", 5, "Explicit String Unassigned", "fxn run()->(void){ str x; println(\"str:\" + x); }", "str:\n"),
        test_n("variable", 6, "Explicit Bool Unassigned", "fxn run()->(void){ bool x; println(x); }", "null\n"),
        test_n("variable", 7, "Explicit Int Assignment", "fxn run()->(void){ int x; x = 5; println(x); }", "5\n"),
        test_n("variable", 8, "Explicit Float Assignment", "fxn run()->(void){ float x; x = 5.5; println(x); }", "5.500000\n"),
        test_n("variable", 9, "Explicit String Assignment", "fxn run()->(void){ str x; x = \"hello\"; println(x); }", "hello\n"),
        test_n("variable", 10, "Explicit Bool Assignment", "fxn run()->(void){ bool x; x = true; println(x); }", "true\n"),
        test_n("variable", 11, "Type Mismatch Error", "fxn run()->(void){ int x; x = 5.5; println(x); }", "1 Semantic Errors"),
    ]

    
    for test in TESTS:
        TESTS[test]

    print(f"Variable test: DONE.")

    