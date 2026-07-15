#!/usr/bin/env bash

##################################################################################################
###                  NOTE: THIS FILE ONLY RUNS ON Linux or GITBASH                             ###                                                               
##################################################################################################


NORMAL="\e[0m"
WARNING="\e[33m"
INFO="\e[34m"
SUCCESS="\e[32m"
ERROR="\e[31m"
INPUT="\e[36m"



VALID_TESTS=("println"  "conditionals"  "variable" "fxns" )


exec_doctest() {
    mkdir -pv temp/$1
    echo -e "\n${INFO}=============================Testing $1=============================${NORMAL}\n"
    python -m doctest -v $1.py
    echo -e "\n${SUCCESS}=============================$1 test done=============================${NORMAL}\n"
}

if [[ $# > 1 ]]; then
    echo "Only 1 args allowed"
    echo "Got $#"
    exit 1

fi

cd tests

echo -e "\n${INFO}RUNNING APOLLO DIAGNOSIS$NORMAL\n"


if [ $# -eq 0 ]; then

    for test in "${!VALID_TESTS[@]}"; do
        exec_doctest "${VALID_TESTS[$test]}"
    done

    echo -e "\n${SUCCESS}DIAGNOSIS COMPLETE.${NORMAL}\n"

    read -p "Delete 'temp' folder in tests? (Y/N): " del

    if [[ $del = "Y" ]]; then
        rm -rfv temp
        echo -e "\nType: 'diagonisis.sh help' for assistance"
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
            echo -e "\n${INFO}=============================TESTING $1=============================${NORMAL}\n"
            python -m doctest -v $1.py
            echo -e "\n${SUCCESS}=============================DIAGNOSIS COMPLETE.=============================${NORMAL}\n"

            read -p "Delete 'temp' folder in tests? (Y/N): " del

            if [[ $del = "Y" ]]; then
                rm -rfv temp

            elif [[ $del = "N" ]]; then
                exit 0

            else 
                echo -e "\n${ERROR}Invalid input, deleting by default."
                rm -rfv temp
                exit 0
            fi  

        

        elif [[ "$1" == "performance" ]]; then
            echo -e "${WARNING}PLEASE NOTE: RUNNING THROUGH BASH DOESN'T PRODUCE ACCURATE RESULTS. $NORMAL\n"
            read -p "Would you still like to continue? (Y/N): " c

            if [[ $c = "N" ]]; then
                exit 0
            
            fi
            cd performance
            python benchmark.py
        
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
    echo "2) conditionals --------------------------------> tests conditionals"
    echo "3) var -----------------------------------------> tests variables"
    echo "4) fxns ----------------------------------------> tests fxns"
    echo "5) performance ----------------------------------> compiler performance"

    exit 0
fi