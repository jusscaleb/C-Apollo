#include <stdio.h>
#include <stdlib.h>
#include <direct.h>


int main(){
    _mkdir("../run");

    int run = system("gcc ../src/main.c ../src/lexer.c ../src/parser.c ../src/generator.c -o ../run/main.exe");

    (run != 0) ? perror("Could not build program") : system("..\\run\\main.exe");

    return 0;



}
