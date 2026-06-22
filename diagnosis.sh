#!/usr/bin/env bash

##################################################################################################
###                  NOTE: THIS FILE ONLY RUNS ON Linux or GITBASH                             ###                                                               
##################################################################################################

VALID_TESTS=("println","conditionals")


if [[ $# > 1 ]]; then
    echo "Only 1 args allowed"
    echo "Got $#"
    exit 1

fi


cd tests


if [ $# -eq 0 ]; then

echo "RUNNING APOLLO DIAGNOSIS"


echo "TESTING println():"
python -m doctest -v println.py

echo "println() test done.\n"

echo "TESTING conditionals"

python -m doctest -v conditionals.py

echo "conditionals test done"

echo "DIAGNOSIS COMPLETE."
echo "Type: 'diagonisis.sh help' for assistance"
exit 0

else 

    if [[ $1 != 'help' ]]; then

    if [[ "${VALID_TESTS[*]}" =~ "$1" ]]; then

        echo "TESTING $1"
        python -m doctest -v $1.py
        echo "DIAGNOSIS COMPLETE."
        exit 0
    
    else
        echo "Unrecognized test args"
        echo "Type: 'diagonisis.sh help' for assistance"
        exit 1

    fi

    fi

fi

if [ $1 = 'help' ]; then
    echo "---------------------DIAGNOSIS MENU---------------------------"
    echo "1) println --------------------------------> tests println()"
    echo "2) println --------------------------------> tests conditionals"

    exit 0

fi

