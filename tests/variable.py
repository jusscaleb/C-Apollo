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
        
    ]

    
    for test in TESTS:
        TESTS[test]

    print(f"Variable test: DONE.")

    