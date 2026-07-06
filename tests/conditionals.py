#########################
##                     ##
##    CONDITIONALS     ##
##                     ##    
#########################

##################
### INCOMPLETE ###
##################
from test import test_n

def run_conditionals_tests():
    """
    >>> run_conditionals_tests()
    conditionals test: 7/7 passed.

    """
    TESTS = [
    test_n("conditionals",1,"if Statement", "fxn run() -> (void) { var name = \"Apollo\"; \n if(name == \"Apollo\"){println(name);} }", "Apollo"),
    test_n("conditionals",2,"if -> else", "fxn run() -> (void) { var name = \"Apollo\"; name = \"Apl\"; if(name == \"Apollo\"){println(\"Apollo\");}else{println(name);} }", "Apl"),
    test_n("conditionals",3,"Variable", "fxn run() -> (void) { var name = \"Apollo\"; var year = 2026; println(name + \" was made in \" + year); }", 
    "Apollo was made in 2026"),
    test_n("conditionals",4, "Sum", "fxn run() -> (void) { println(5 + 10); }", "15"),
    test_n("conditionals",5, "Product", "fxn run() -> (void) { println(5 * 10); }", "50"),
    test_n("conditionals",6, "Quotient", "fxn run() -> (void) { println(10 / 5); }", "2"),
    test_n("conditionals" ,7, "Modulo", "fxn run() -> (void) { println(10 % 5); }", "0"),
    ]


    pass_count = 0
    for test in TESTS:
        if  TESTS[test] == 1:
            pass_count += 1

    print(f"conditionals test: {pass_count}/{len(TESTS)} passed.")

    