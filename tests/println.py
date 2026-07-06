#########################
##                     ##
##    PRINTLN() TEST   ##
##                     ##    
#########################

#run "python -m doctest println.py"

from test import test_n



def run_println_tests():
    """
    >>> run_println_tests()
    println test: DONE.
    """
    TESTS = [

    test_n("println",
    1,
    "String Literals", 
    "### STRING LITERALS TEST ###\n"
    "fxn run() -> (void) { println(\"hello world\");" 
    "}"
    , "hello world"),


    test_n("println",2,"Integers", 
    "### INTEGERS ###\n"
    "fxn run() -> (void) {" 
    "println(5);"
    "}", "5"),

    test_n("println",3,"Variable", 
    "### CONCATENATION ###\n"
    "fxn run() -> (void) { var name = \"Apollo\"; var year = 2026; println(name + \" was made in \" + year); }", 
    "Apollo was made in 2026"),

    test_n("println",4, "Sum", 
    
    "### ARITHMETIC ###\n"
    "fxn run() -> (void) { println(5 + 10); }", "15"),

    test_n("println",5, "Product", 
    
    "### PRODUCT ###\n"
    "fxn run() -> (void) { println(5 * 10); }", "50"),
    test_n("println",6, "Quotient", 
    "### QUOTIENT ###\n"
    "fxn run() -> (void) { println(10 / 5); }", "2"),
    
    test_n("println",7, "Modulo", 
    "### MODULUS ###\n"
    "fxn run() -> (void) { println(\"Mod: \" + 10 % 5); }", "Mod: 0"),

    ### ESCAPE CHARACTERS ###
    test_n("println",8, "Escape", 
    "### ESCAPE CHARACTERS ###\n"
    "fxn run() -> (void) { println(\"Newline: \\nTab: \\tskipped\\nQuote: \\\"quote\\\" \"); }", "Newline: \nTab: \tskipped\nQuote: \"quote\""),
    
    ### BOOLEANS ###
    test_n("println",9, "Escape", 
    "### BOOLEANS ###\n"
    "fxn run() -> (void) { println(null); println(true);\nprintln(false);}", "null\ntrue\nfalse"),    
    ]

    for test in TESTS:
        TESTS[test]

    print(f"println test: DONE.")

    