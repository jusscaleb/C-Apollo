#!/usr/bin/env bash

##################################################################################################
###                  NOTE: THIS FILE ONLY RUNS ON Linux or GITBASH                             ###                                                               
##################################################################################################

VALID_TESTS=("println"  "conditionals"  "variable" "fxns")


exec_doctest() {
    mkdir -pv temp/$1
    echo "Testing $1"
    python -m doctest -v $1.py
    echo "$1 test done"
}

if [[ $# > 1 ]]; then
    echo "Only 1 args allowed"
    echo "Got $#"
    exit 1

fi

cd tests


if [ $# -eq 0 ]; then
    echo "RUNNING APOLLO DIAGNOSIS"

    for test in "${!VALID_TESTS[@]}"; do
        exec_doctest "${VALID_TESTS[$test]}"
    done

    echo "DIAGNOSIS COMPLETE."

    read -p "Delete 'temp' folder in tests? (Y/N): " del

    if [[ $del = "Y" ]]; then
        rm -rfv temp
        echo "Type: 'diagonisis.sh help' for assistance"
        exit 0
    
    elif [[ $del = "N" ]]; then
        exit 1
    
   
    else 
        echo "Invalid input, deleting by default"
        rm -rfv temp
        exit 0
    fi  

else 

    if [[ $1 != 'help' ]]; then

        if [[ "${VALID_TESTS[*]}" =~ "$1" ]]; then

            mkdir -p -v temp/$1
            echo "TESTING $1"
            python -m doctest -v $1.py
            echo "DIAGNOSIS COMPLETE."

            read -p "Delete 'temp' folder in tests? (Y/N): " del

            if [[ $del = "Y" ]]; then
                rm -rfv temp

            elif [[ $del = "N" ]]; then
                exit 0

            else 
                echo "Invalid input, deleting by default"
                rm -rfv temp
                exit 0
            fi  

        fi   
    
    else
        echo "Unrecognized test args"
        echo "Type: 'diagonisis.sh help' for assistance"
        exit 1

    fi

fi

if [ $1 = 'help' ]; then
    echo "---------------------DIAGNOSIS MENU---------------------------"
    echo "1) println --------------------------------> tests println()"
    echo "2) conditionals --------------------------------> tests conditionals"
    echo "3) var -----------------------------------------> tests variables"
    echo "4) fxns ----------------------------------------> tests fxns"

    exit 0
fi