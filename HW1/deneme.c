#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int divide_string(char *string, char **tokens){
    
    char *token = strtok(string, " ");
    tokens[0] = token;
    int i = 1;
    while(token != NULL){
        token = strtok(NULL, "\"");
        tokens[i] = token;
        token = strtok(NULL, "\"");
        i++;
    }
    return i;
}

int main(){
    
    char string[]  = "gtuStudentGrades \"file_name\" \"Burak Kocak\" \"123456\"";
    char **tokens = (char **)malloc(100 * sizeof(char *));
    int num_of_tokens = divide_string(string, tokens);

    //print all tokens
    for (int i = 0; i < num_of_tokens; i++){
        printf("tokens[%d]: %s\n", i, tokens[i]);
    } 
}