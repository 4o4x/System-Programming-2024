#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/types.h>

// StudentGrade struct
typedef struct StudentGrade{
    char *student_id;
    char *grade;
} StudentGrade;

// Create a new StudentGrade
StudentGrade *create_student_grade(char *student_id, char *grade){
    // Allocate memory for the StudentGrade
    StudentGrade *student_grade = (StudentGrade *)malloc(sizeof(StudentGrade));

    // Copy the student id and grade to the StudentGrade
    student_grade->student_id = strdup(student_id);
    student_grade->grade = strdup(grade);
    return student_grade;
}

StudentGrade *create_student_grade_from_line(char *line){
    char *token = strtok(line, ",");
    char *student_id = token;
    token = strtok(NULL, ",");
    char *grade = token;
    return create_student_grade(student_id, grade);
}


// Free the memory of the StudentGrade

void free_student_grade(StudentGrade *student_grade){
    free(student_grade->student_id);
    free(student_grade->grade);
    free(student_grade);
}

// Print the StudentGrade
void print_student_grade(StudentGrade *student_grade){
    printf("Student Name and Surname: %s\nGrade: %s\n", student_grade->student_id, student_grade->grade);
}

// Vector of StudentGrade

typedef struct StudentGradeVector{
    StudentGrade **student_grades;
    int size;
    int capacity;
} StudentGradeVector;

// Create a new StudentGradeVector
StudentGradeVector *create_student_grade_vector(){
    StudentGradeVector *student_grade_vector = (StudentGradeVector *)malloc(sizeof(StudentGradeVector));
    student_grade_vector->size = 0;
    student_grade_vector->capacity = 100;
    student_grade_vector->student_grades = (StudentGrade **)malloc(student_grade_vector->capacity * sizeof(StudentGrade *));
    return student_grade_vector;
}


// Add a new StudentGrade to the StudentGradeVector
void add_student_grade_to_vector(StudentGradeVector *student_grade_vector, StudentGrade *student_grade){
    if (student_grade_vector->size == student_grade_vector->capacity){
        student_grade_vector->capacity *= 2;
        student_grade_vector->student_grades = (StudentGrade **)realloc(student_grade_vector->student_grades, student_grade_vector->capacity * sizeof(StudentGrade *));
    }
    student_grade_vector->student_grades[student_grade_vector->size] = student_grade;
    student_grade_vector->size++;
}

// Free the memory of the StudentGradeVector
void free_student_grade_vector(StudentGradeVector *student_grade_vector){
    for (int i = 0; i < student_grade_vector->size; i++){
        free(student_grade_vector->student_grades[i]->student_id);
        free(student_grade_vector->student_grades[i]->grade);
        free(student_grade_vector->student_grades[i]);
    }
    free(student_grade_vector->student_grades);
    free(student_grade_vector);
}

// Print the StudentGradeVector
void print_student_grade_vector(StudentGradeVector *student_grade_vector){
    for (int i = 0; i < student_grade_vector->size; i++){
        print_student_grade(student_grade_vector->student_grades[i]);
    }
}


// Vector of Strings

typedef struct StringVector{
    char **strings;
    int size;
    int capacity;
} StringVector;

// Create a new StringVector
StringVector *create_string_vector(){
    StringVector *string_vector = (StringVector *)malloc(sizeof(StringVector));
    string_vector->size = 0;
    string_vector->capacity = 100;
    string_vector->strings = (char **)malloc(string_vector->capacity * sizeof(char *));
    return string_vector;
}

// Add a new string to the StringVector
void add_string_to_vector(StringVector *string_vector, char *string){
    if (string_vector->size == string_vector->capacity){
        string_vector->capacity *= 2;
        string_vector->strings = (char **)realloc(string_vector->strings, string_vector->capacity * sizeof(char *));
    }
    string_vector->strings[string_vector->size] = strdup(string);
    string_vector->size++;
}

// Free the memory of the StringVector
void free_string_vector(StringVector *string_vector){
    for (int i = 0; i < string_vector->size; i++){
        free(string_vector->strings[i]);
    }
    free(string_vector->strings);
    free(string_vector);
}





int logging(char *log){

    // Open the log file
    int fd = open("log.txt", O_CREAT | O_WRONLY | O_APPEND, 0644);

    // Check if the file is opened successfully otherwise print an error message and return -1
    if (fd == -1){
        perror("Cannot open log file");
        return -1;
    }

    // Write the log to the file
    write(fd, log, strlen(log));

    // Close the file
    close(fd);
    return 0;
}

int create_file(char *file_name){

    // Open the file
    int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    // Check if the file is opened successfully otherwise print an error message and return -1
    if (fd == -1){
        perror("File cannot be created");
        return -1;
    }
    // Close the file
    close(fd);
    return 0;
}

int add_student_grade(char *file_name, char *student_id, char *grade){
    
    // Open the file
    int fd = open(file_name, O_WRONLY | O_APPEND);
    
    // Check if the file is opened
    if (fd == -1){
        perror("open");
        return -1;
    }
    
    // Write the student id and grade to the file
    char *buf = (char *)malloc(100);
    sprintf(buf, "%s,%s\n", student_id, grade);
    int write_check = write(fd, buf, strlen(buf));
    
    // Check if the write operation is successful
    if (write_check == -1){
        perror("write");
        return -1;
    }

    // Close the file
    close(fd);

    // Free the buffer
    free(buf);
    return 0;
}

int search_student(char *file_name, char *student_name_surname){
    int fd = open(file_name, O_RDONLY);
    if (fd == -1){
        perror("open");
        return -1;
    }

    
    int n;
    char buffer[100];
    char line[100];
    int lineIndex = 0;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineIndex] = '\0'; // Null-terminate the line
                printf("%s\n", line);   // Print the line

                // Divide the line into tokens
                char *token = strtok(line, ",");
                char *student_id = token;
                token = strtok(NULL, ",");
                char *grade = token;

                // Check if the student name and surname is found
                if (strcmp(student_id, student_name_surname) == 0){
                    printf("Student Name and Surname: %s\nGrade: %s\n", student_id, grade);
                    close(fd);
                    return 0;
                }


                lineIndex = 0;          // Reset the line index
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }

    if (n < 0) {
        perror("Failed to read the file");
        return -1;
    }
    
    close(fd);
    return -1;
}




// Compare the grades in ascending order
// Type: StudentGrade
int compare_grades_ascending(const void *a, const void *b){
    StudentGrade *grade_a = *(StudentGrade **)a;
    StudentGrade *grade_b = *(StudentGrade **)b;
    return strcmp(grade_a->grade, grade_b->grade);
}

// Compare the grades in descending order
// Type: StudentGrade
int compare_grades_descending(const void *a, const void *b){
    StudentGrade *grade_a = *(StudentGrade **)a;
    StudentGrade *grade_b = *(StudentGrade **)b;
    return strcmp(grade_b->grade, grade_a->grade);
}

// Compare the names in ascending order
// Type: StudentGrade
int compare_names_ascending(const void *a, const void *b){
    StudentGrade *name_a = *(StudentGrade **)a;
    StudentGrade *name_b = *(StudentGrade **)b;
    return strcmp(name_a->student_id, name_b->student_id);
}

// Compare the names in descending order
// Type: StudentGrade
int compare_names_descending(const void *a, const void *b){
    StudentGrade *name_a = *(StudentGrade **)a;
    StudentGrade *name_b = *(StudentGrade **)b;
    return strcmp(name_b->student_id, name_a->student_id);
}

int sort_student(char *file_name, char *option, char *order){
    int fd = open(file_name, O_RDWR);
    if (fd == -1){
        perror("open");
        return -1;
    }

    printf("option: %s, order: %s\n", option, order);

    // Read the file
    int n;
    char buffer[100];
    char line[100];
    int lineIndex = 0;
    int num_of_lines = 0;

    int total_size = 100;

    // Create a StudentGrade vector
    StudentGrade **student_grades = create_student_grade_vector();

    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineIndex] = '\0'; // Null-terminate the line
                
                //printf("%s\n", line);   // Print the line
                //printf("student_id: %s, grade: %s\n", student_id, grade);

                // Add the student grade to the vector
                add_student_grade_to_vector(student_grades, create_student_grade_from_line(line));
            
                num_of_lines++;
                lineIndex = 0;          // Reset the line index
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }

    if (n < 0) {
        perror("Failed to read the file");
        return -1;
    }

    // Print the sorted student names and grades
    printf("Before sorting\n");
    
    for (int i = 0; i < num_of_lines; i++){
        printf("student_id: %s, grade: %s\n", student_grades[i]->student_id, student_grades[i]->grade);
    }


    // Sort the student names and grades
    if (strcmp(option, "-grade") == 0){
        if (strcmp(order, "-a") == 0){
            qsort(student_grades, num_of_lines, sizeof(StudentGrade *), compare_grades_ascending);
        }
        else if (strcmp(order, "-d") == 0){
            qsort(student_grades, num_of_lines, sizeof(StudentGrade *), compare_grades_descending);
        }
    }
    else if (strcmp(option, "-names") == 0){
        if (strcmp(order, "-a") == 0){
            qsort(student_grades, num_of_lines, sizeof(StudentGrade *), compare_names_ascending);
        }
        else if (strcmp(order, "-d") == 0){
            qsort(student_grades, num_of_lines, sizeof(StudentGrade *), compare_names_descending);
        }
    }

    // Print the sorted student names and grades
    printf("After sorting\n");

    for (int i = 0; i < num_of_lines; i++){
        printf("student_id: %s, grade: %s\n", student_grades[i]->student_id, student_grades[i]->grade);
    }



    // free the memory
    for (int i = 0; i < num_of_lines; i++){
        free(student_grades[i]->student_id);
        free(student_grades[i]->grade);
        free(student_grades[i]);
    }
    free(student_grades);


    close(fd);

    return 0;
    
}


int show_all(char *file_name){
    int fd = open(file_name, O_RDONLY);
    if (fd == -1){
        perror("open");
        return -1;
    }
    int n;
    char buffer[100];
    char line[100];
    int lineIndex = 0;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineIndex] = '\0'; // Null-terminate the line
                printf("%s\n", line);   // Print the line

                lineIndex = 0;          // Reset the line index
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }
    
    close(fd);
    return 0;
}

int list_grades(char *file_name){
    int fd = open(file_name, O_RDONLY);
    if (fd == -1){
        perror("open");
        return -1;
    }
    int n;
    char buffer[100];
    char line[100];
    int lineIndex = 0;
    int j = 0;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineIndex] = '\0'; // Null-terminate the line
                printf("%s\n", line);   // Print the line

                lineIndex = 0;         // Reset the line index
                j++;
                if (j == 5){
                    close(fd);
                    return 0;
                }
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }
    
    close(fd);
    return 0;
}

int list_some(char *file_name, int num_of_entries, int page_number){
    int fd = open(file_name, O_RDONLY);
    if (fd == -1){
        perror("open");
        return -1;
    }
    int n;
    char buffer[100];
    char line[100];
    int lineIndex = 0;
    int first = page_number * 5 - 1;
    int last = first + num_of_entries;
    int j = 0;
    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                line[lineIndex] = '\0'; // Null-terminate the line
                printf("%s\n", line);   // Print the line

                if (j >= first && j < last){
                    printf("\nFound %s\n", line);
                    close(fd);
                    return 0;
                }

                lineIndex = 0;         // Reset the line index
                j++;
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }

    close(fd);
    return 0;
}


// Divide the string into tokens
// First token is the command
// Other token format is "argument"
// ex: gtuStudentGrades "grades.txt" -> gtuStudentGrades, grades.txt
// ex: addStudentGrade "grades.txt" "John Doe" "100" -> addStudentGrade, grades.txt, John Doe, 100

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

    while(1){
        char string[100];
        printf("Command (type 'exit' to exit): ");
        char *check = fgets(string, 100, stdin);

        if (check == NULL)
        {
            perror("fgets");
            exit(1);
        }
        

        string[strlen(string) - 1] = '\0';
        
        printf("string: %s\n", string);
        printf("string length: %ld\n", strlen(string));

        if(strcmp(string, "exit") == 0) break;


        //The user should be able to d'splay all of the ava'lable commands by call'ng gtuStudentGrades without an argument.
        else if(strcmp(string, "gtuStudentGrades") == 0){
            printf("\n\nAvailable commands:\n\n");

            printf("-------------------------------------------\n\n");

            printf("gtuStudentGrades\n\n");
            printf("Description: Display all of the available commands\n\n");
            printf("Example Usage: gtuStudentGrades\n\n");

            printf("-------------------------------------------\n\n");

            printf("gtuStudentGrades \"<file_name>\"\n\n");
            printf("Description: Create a new file\n\n");
            printf("Example Usage: gtuStudentGrades \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("addStudentGrade \"<file_name>\" \"<student_name_surname>\" \"<grade>\"\n\n");
            printf("Description: Add a new student's grade to the system\n\n");
            printf("Example Usage: addStudentGrade \"grades.txt\" \"John Doe\" \"AA\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("searchStudent \"<file_name>\" \"<student_name_surname>\"\n\n");
            printf("Description: Search for a student's grade and display the student name and grade\n\n");
            printf("Example Usage: searchStudent \"grades.txt\" \"John Doe\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("sortAll \"<file_name>\" \"<option>\" \"<order>\"\n\n");
            printf("Description: Sort the student grades in the file\n\n");
            printf("Example Usage: sortAll \"grades.txt\" \"-grade\" \"-a\"\n\n");
            printf("Parameters:\n");
            printf("<option>: -grade: Sort the grades or -names: Sort the student names\n");
            printf("<order>: -a: Sort the grades in ascending order or -d: Sort the grades in descending order\n\n");

            printf("-------------------------------------------\n\n");

            printf("showAll \"<file_name>\"\n\n");
            printf("Description: Display all student grades stored in the file\n\n");
            printf("Example Usage: showAll \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("listGrades \"<file_name>\"\n\n");
            printf("Description: List first 5 entries of the file\n\n");
            printf("Example Usage: listGrades \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("listSome \"<num_of_entries>\" \"<page_number>\" \"<file_name>\"\n\n");
            printf("Description: List the entries of the file\n\n");
            printf("Example Usage: listSome \"5\" \"1\" \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("exit\n\n");
            printf("Description: Exit the program\n\n");
            printf("Example Usage: exit\n\n");

            printf("-------------------------------------------\n\n");
        }

        
        //Menu

        // Create file
        // The user should be able to create a new file by calling gtuStudentGrades with the following arguments:
        // gtuStudentGrades <file_name>
        
        else if(strncmp(string, "gtuStudentGrades", 16) == 0){
            
            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

            if (num_of_tokens != 2){
                printf("Invalid command\n");
                continue;
            }

            //print all tokens
            for (int i = 0; i < num_of_tokens; i++){
                printf("tokens[%d]: %s\n", i, tokens[i]);
            }

            char *file_name = tokens[1];
            
            printf("file_name: %s\n", file_name);

            if (fork() == 0){
                int return_value = create_file(file_name);
                if (return_value == -1){
                    printf("File cannot be created\n");
                }
                else printf("File created\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }
        }

        

        // Add Student Grade: The user should be able to add a new student's grade to the system. 
        // Add a student grade to the file. If the file does not exist, the program should print an error message.
        // The user should be able to add a student grade by calling addStudentGrade with the following arguments:
        // addStudentGrade <file_name> <student_name_surname> <grade>
        else if(strncmp(string, "addStudentGrade", 15) == 0){
            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

            //print all tokens
            for (int i = 0; i < num_of_tokens; i++){
                printf("tokens[%d]: %s\n", i, tokens[i]);
            }

            if (num_of_tokens != 4){
                printf("Invalid command\n");
                continue;
            }

            char *file_name = tokens[1];
            char *student_id = tokens[2];
            char *grade = tokens[3];

            if (fork() == 0){
                int return_value = add_student_grade(file_name, student_id, grade);
                if (return_value == -1){
                    printf("Student grade cannot be added\n");
                }
                else printf("Student grade added\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }

    
        }

        // Search for Student Grade
        // The user should be able to search for a student's grade by calling searchStudent with the following arguments:
        // searchStudent <file_name> <student_name_surname>

        else if(strncmp(string, "searchStudent", 13) == 0){
            char **tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);
            if (num_of_tokens != 3){
                printf("Invalid command\n");
                continue;
            }
            char *file_name = tokens[1];
            char *student_name_surname = tokens[2];
            if (fork() == 0){
                int return_value = search_student(file_name, student_name_surname);
                if (return_value == -1){
                    printf("Student grade cannot be found\n");
                }
                else printf("Student grade found\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }
        }

        // Sort Student Grades:
        // The user should be able to sort the student grades in the file by calling sortAll with the following arguments
        // sortAll <file_name> 
        // Options: 
        // -grade: Sort the grades or -names: Sort the student names 
        // -a: Sort the grades in ascending order or -d: Sort the grades in descending order
        
        else if(strncmp(string, "sortAll", 7) == 0){
            
            char **tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);

            for (int i = 0; i < num_of_tokens; i++){
                printf("tokens[%d]: %s\n", i, tokens[i]);
            }
            
            char *file_name = tokens[1];
            char *option = tokens[2];
            char *order = tokens[3];


            if (fork() == 0){
                int return_value = sort_student(file_name, option, order);
                if (return_value == -1){
                    printf("Student grades cannot be sorted\n");
                }
                else printf("Student grades sorted\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }
        }


        // Display Student Grades:
        // The user should be able to display all student grades stored in the file. 
        // The program should display the student name and grade for each student. Also display content page by page and first 5 entries.
        // showAll <file_name>
        else if(strncmp(string, "showAll", 7) == 0){
            char ** tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);
            if (num_of_tokens != 2){
                printf("Invalid command\n");
                continue;
            }

            char *file_name = tokens[1];

            if (fork() == 0){
                int return_value = show_all(file_name);
                if (return_value == -1){
                    printf("Student grades cannot be displayed\n");
                }
                else printf("Student grades displayed\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }

        }

        // listGrades <file_name>
        // The user should be able to list first 5 entries of the file.
        else if(strncmp(string, "listGrades", 10) == 0){
            char **tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);
            if (num_of_tokens != 2){
                printf("Invalid command\n");
                continue;
            }
            char *file_name = tokens[1];
            if (fork() == 0){
                int return_value = list_grades(file_name);
                if (return_value == -1){
                    printf("Student grades cannot be listed\n");
                }
                else printf("Student grades listed\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }
        }

        // listSome <num_of_entries> <page_number> <file_name>
        // The user should be able to list the entries of the file.
        else if(strncmp(string, "listSome", 8) == 0){
            char **tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);
            if (num_of_tokens != 4){
                printf("Invalid command\n");
                continue;
            }
            int num_of_entries = atoi(tokens[1]);
            int page_number = atoi(tokens[2]);
            char *file_name = tokens[3];
            if (fork() == 0){
                int return_value = list_some(file_name, num_of_entries, page_number);
                if (return_value == -1){
                    printf("Student grades cannot be listed\n");
                }
                else printf("Student grades listed\n");
            }
            else{
                waitpid(-1, NULL, 0);
            }
        }


        // Invalid command
        else printf("Invalid command\n");
    }
    
    return 0;
}