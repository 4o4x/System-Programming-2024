#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <errno.h>
#include <time.h>



// StudentGrade struct
typedef struct Student{
    char *name; // Name and surname of the student
    char *grade; // Grade of the student
} Student;

// Create a new Student
Student *create_student(char *name, char *grade){
    Student *student = (Student *)malloc(sizeof(Student));
    student->name = strdup(name);
    student->grade = strdup(grade);
    return student;
}

// Create a new Student from line of the file
Student *create_student_from_line(char *line){
    char *token = strtok(line, ",");
    char *name = token;
    token = strtok(NULL, ",");
    char *grade = token;
    return create_student(name, grade);
}


// Free the memory of the StudentGrade
void free_student(Student *student){
    free(student->name);
    free(student->grade);
    free(student);
}

// Print the StudentGrade
void print_student(Student *student){
    printf("Name: %s, Grade: %s\n", student->name, student->grade);
}

// Vector of Student

typedef struct StudentVector{
    Student **students;
    int size;
    int capacity;
} StudentVector;

// Create a new StudentVector
StudentVector *create_student_vector(){
    StudentVector *student_vector = (StudentVector *)malloc(sizeof(StudentVector));
    student_vector->size = 0;
    student_vector->capacity = 100;
    student_vector->students = (Student **)malloc(student_vector->capacity * sizeof(Student *));
    return student_vector;
}


// Add a new Student to the StudentVector
void add_student_to_vector(StudentVector *student_vector, Student *student){
    if (student_vector->size == student_vector->capacity){
        student_vector->capacity *= 2;
        student_vector->students = (Student **)realloc(student_vector->students, student_vector->capacity * sizeof(Student *));
    }
    student_vector->students[student_vector->size] = student;
    student_vector->size++;
}

// Compare the grades in ascending order
// Type: Student
int compare_grades_ascending(const void *a, const void *b){
    Student *student_a = *(Student **)a;
    Student *student_b = *(Student **)b;
    return strcmp(student_a->grade, student_b->grade);
}

// Compare the grades in descending order
// Type: Student
int compare_grades_descending(const void *a, const void *b){
    Student *student_a = *(Student **)a;
    Student *student_b = *(Student **)b;
    return strcmp(student_b->grade, student_a->grade);
}

// Compare the names in ascending order
// Type: Student
int compare_names_ascending(const void *a, const void *b){
    Student *student_a = *(Student **)a;
    Student *student_b = *(Student **)b;
    return strcmp(student_a->name, student_b->name);
}

// Compare the names in descending order
// Type: Student

int compare_names_descending(const void *a, const void *b){
    Student *student_a = *(Student **)a;
    Student *student_b = *(Student **)b;
    return strcmp(student_b->name, student_a->name);
}

//Sort the StudentVector
void sort_student_vector(StudentVector *student_vector, char *option, char *order){
    if (strcmp(option, "grade") == 0){
        if (strcmp(order, "a") == 0){
            qsort(student_vector->students, student_vector->size, sizeof(Student *), compare_grades_ascending);
        }
        else if (strcmp(order, "d") == 0){
            qsort(student_vector->students, student_vector->size, sizeof(Student *), compare_grades_descending);
        }
    }
    else if (strcmp(option, "name") == 0){
        if (strcmp(order, "a") == 0){
            qsort(student_vector->students, student_vector->size, sizeof(Student *), compare_names_ascending);
        }
        else if (strcmp(order, "d") == 0){
            qsort(student_vector->students, student_vector->size, sizeof(Student *), compare_names_descending);
        }
    }
}



// Free the memory of the StudentVector
void free_student_vector(StudentVector *student_vector){
    for (int i = 0; i < student_vector->size; i++){
        free_student(student_vector->students[i]);
    }
    free(student_vector->students);
    free(student_vector);
}


// Print the StudentVector
void print_student_vector(StudentVector *student_vector){
    for (int i = 0; i < student_vector->size; i++){
        print_student(student_vector->students[i]);
    }
}



int logging(char *command, char *status, char *result){

    time_t t;   // not a primitive datatype
    time(&t);
    

    pid_t pid = fork();

    if (pid < 0){
        perror("fork");
        return -1;
    }

    else if (pid == 0){
        
        int fd = open("app.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
        
        if (fd == -1){
            perror("Log file cannot be opened");
            exit(1);
        }

        char *buf = (char *)malloc(200);
        sprintf(buf, "%s,%s,%s,%s", command, status, result, ctime(&t));
        free(command);
        int write_check = write(fd, buf, strlen(buf));

        free(buf);
        
        if (write_check == -1){
            perror("write");  
            close(fd);
            exit(1);
        }
        
        close(fd);
        exit(0);
    }
    else{
        waitpid(-1, NULL, 0);
        return 0;
    }

    return 0;

}


// Create a file
// Return 0 if the file is created successfully
// Return -1 if the file cannot be created 

int create_file(char *file_name){

    // Open the file
    int fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    // Check if the file is opened successfully 
    // Otherwise print an error message and return -1
    if (fd == -1){
        perror("File cannot be created");
        return -1;
    }
    // Close the file
    close(fd);
    return 0;
}


// Add a student grade to the file
// Return 0 if the student grade is added successfully
// Return -1 if the file cannot be opened
// Return -2 if the write operation is not successful

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
        close(fd);
        return -1;
    }

    // Close the file
    close(fd);

    // Free the buffer
    free(buf);
    return 0;
}

// Search for a student's grade and display the student name and grade
// Return 0 if the student grade is found
// Return -1 if the file cannot be opened
// Return -2 if the read operation is not successful
// Return -3 if the student grade cannot be found


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

                Student *student = create_student_from_line(line);
                if (strcmp(student->name, student_name_surname) == 0){
                    print_student(student);
                    free_student(student);
                    close(fd);
                    return 0;
                }

                free_student(student);


                lineIndex = 0;          // Reset the line index
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }

    if (n < 0) {
        perror("Failed to read the file");
        close(fd);
        return -2;
    }
    
    close(fd);
    return -3;
}

// Sort the students in the file
// Return 0 if the students are sorted
// Return -1 if the file cannot be opened
// Return -2 if the read operation is not successful

int sort_student(char *file_name, char *option, char *order){
    
    int fd = open(file_name, O_RDONLY);
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

    // Create a StudentVector
    StudentVector *student_grades = create_student_vector();

    while ((n = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        for (int i = 0; i < n; i++) {
            char c = buffer[i];
            if (c == '\n' || c == '\0') {
                
                line[lineIndex] = '\0'; 
                
                // Add the student grade to the vector 
                add_student_to_vector(student_grades, create_student_from_line(line));
            
                lineIndex = 0;          
                continue;
            }
            // Add the character to the line
            line[lineIndex++] = c; 
        }
    }

    if (n < 0) {
        perror("Failed to read the file");
        free_student_vector(student_grades);
        close(fd);
        return -2;

    }

    // Sort the StudentVector
    sort_student_vector(student_grades, option, order);

    print_student_vector(student_grades);
    
    //free the memory of the StudentVector
    free_student_vector(student_grades);
    

    // Close the file
    close(fd);

    return 0;
    
}

// Display all student grades stored in the file
// Return 0 if the student grades are displayed
// Return -1 if the file cannot be opened
// Return -2 if the read operation is not successful
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
                
                Student *student = create_student_from_line(line);
                print_student(student);
                free_student(student);

                lineIndex = 0;          // Reset the line index
                continue;
            }
            line[lineIndex++] = c; // Add the character to the line
        }
    }

    if (n < 0) {
        perror("Failed to read the file");
        close(fd);
        return -2;
    }
    
    close(fd);
    return 0;
}

// List first 5 entries of the file
// Return 0 if the student grades are listed
// Return -1 if the file cannot be opened
// Return -2 if the read operation is not successful
// Return -3 if there are less than 5 entries in the file

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
                
                Student *student = create_student_from_line(line);
                print_student(student);
                free_student(student);

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

    if (n < 0) {
        perror("Failed to read the file");
        close(fd);
        return -2;
    }

    if( j < 5){
        close(fd);
        return -3;
    }
    
    close(fd);
    return 0;
}


// List the entries of the file
// Return 0 if the student grades are listed
// Return -1 if the file cannot be opened
// Return -2 if the read operation is not successful
// Return -3 if there are not enough entries in the file

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

                if (j >= first && j < last){
                    Student *student = create_student_from_line(line);
                    print_student(student);
                    free_student(student);
                }

                if (j >= last){
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

    if (n < 0) {
        perror("Failed to read the file");
        return -2;
    }

    if( j < last){
        close(fd);
        return -3;
    }

    close(fd);
    return -1;
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
        if (token == NULL){
            break;
        }
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
        
        //printf("string: %s\n", string);
        //printf("string length: %ld\n", strlen(string));

        if(strcmp(string, "exit") == 0){
            char *command = strdup(string);
            logging(command, "success", "Exit the program");
            free(command);
            break;
        }


        //The user should be able to d'splay all of the ava'lable commands by call'ng gtuStudentGrades without an argument.
        else if(strcmp(string, "gtuStudentGrades") == 0){
            char * command = strdup(string);
         
            logging(command, "success", "Display all of the available commands");
            free(command);

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
            printf("<option>: grade: Sort the grades or name: Sort the student names\n");
            printf("<order>: a: Sort the grades in ascending order or d: Sort the grades in descending order\n\n");

            printf("-------------------------------------------\n\n");

            printf("showAll \"<file_name>\"\n\n");
            printf("Description: Display all student grades stored in the file\n\n");
            printf("Example Usage: showAll \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("listGrades \"<file_name>\"\n\n");
            printf("Description: List first 5 entries of the file\n\n");
            printf("Example Usage: listGrades \"grades.txt\"\n\n");

            printf("-------------------------------------------\n\n");

            printf("listSome \"<file_name>\" \"<num_of_entries>\" \"<page_number>\"\n\n");
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

            char *command  = strdup(string);
            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

            if (num_of_tokens != 2){
                printf("Incorrect number of arguments! Expected: 1\n");
                free(tokens);
                logging(command, "fail", "Incorrect number of arguments! Expected: 1");
                continue;
            }


            char *file_name = tokens[1];

            //free the memory of the tokens
            free(tokens);
            
            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                continue;
            }

            else if (pid == 0){
                int return_value = create_file(file_name);
                
                if (return_value == -1){
                    printf("File cannot be created\n");
                    logging(command, "fail", "File cannot be created");
                    free(command);
                    exit(1);
                }
                
                printf("File created\n");
                logging(command, "success", "File created");
                free(command);
                exit(0);
            }
            
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
            

        }

        

        // Add Student Grade: The user should be able to add a new student's grade to the system. 
        // Add a student grade to the file. If the file does not exist, the program should print an error message.
        // The user should be able to add a student grade by calling addStudentGrade with the following arguments:
        // addStudentGrade <file_name> <student_name_surname> <grade>
        else if(strncmp(string, "addStudentGrade", 15) == 0){
            
            char *command  = strdup(string);

            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

           

            if (num_of_tokens != 4){
                printf("Incorrect number of arguments! Expected: 3\n");
                free(tokens);

                logging(command, "fail", "Incorrect number of arguments! Expected: 3");
                free(command);

                continue;
            }

            char *file_name = tokens[1];
            char *student_id = tokens[2];
            char *grade = tokens[3];

            free(tokens);

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }
    
            else if (pid == 0){

                int return_value = add_student_grade(file_name, student_id, grade);

                if (return_value == 0){
                    printf("Student grade added\n");
                    logging(command, "success", "Student grade added");
                    free(command);
                    exit(0);
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Write operation is not successful\n");
                    logging(command, "fail", "Write operation is not successful");
                    free(command);
                    exit(1);
                }
                exit(0); 
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
    
        }

        // Search for Student Grade
        // The user should be able to search for a student's grade by calling searchStudent with the following arguments:
        // searchStudent <file_name> <student_name_surname>

        else if(strncmp(string, "searchStudent", 13) == 0){



            char *command  = strdup(string);

            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

           

            if (num_of_tokens != 3){
                printf("Incorrect number of arguments! Expected: 2\n");
                free(tokens);

                logging(command, "fail", "Incorrect number of arguments! Expected: 2");
                free(command);

                continue;
            }
            
           
            
            char *file_name = tokens[1];
            char *student_name_surname = tokens[2];

            free(tokens);

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }

            else if (pid == 0){
                int return_value = search_student(file_name, student_name_surname);

                
                if (return_value == 0){
                    logging(command, "success", "Student grade found");
                    free(command);
                    exit(0);
                    
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Read operation is not successful\n");
                    logging(command, "fail", "Read operation is not successful");
                    free(command);
                    exit(1);
                }
                else if (return_value == -3){
                    printf("Student grade cannot be found\n");
                    logging(command, "fail", "Student grade cannot be found");
                    free(command);
                    exit(1);
                }
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
        }

        // Sort Student Grades:
        // The user should be able to sort the student grades in the file by calling sortAll with the following arguments
        // sortAll <file_name> 
        // Options: 
        // grade: Sort the grades or name: Sort the student names 
        // a: Sort the grades in ascending order or d: Sort the grades in descending order
        
        else if(strncmp(string, "sortAll", 7) == 0){

            char *command  = strdup(string);
            
            char **tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);

            if (num_of_tokens != 4){
                printf("Incorrect number of arguments! Expected: 3\n");
                free(tokens);
                logging(command, "fail", "Incorrect number of arguments! Expected: 3");
                free(command);
                continue;
            }
            
            char *file_name = tokens[1];
            char *option = tokens[2];
            char *order = tokens[3];

            free(tokens);

            if (strcmp(option, "grade") != 0 && strcmp(option, "name") != 0){
                printf("Invalid option\n");
                logging(command, "fail", "Invalid option");
                free(command);
                continue;
            }

            if (strcmp(order, "a") != 0 && strcmp(order, "d") != 0){
                printf("Invalid order\n");
                logging(command, "fail", "Invalid order");
                free(command);
                continue;
            }
            

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }

            else if (pid == 0){
                int return_value = sort_student(file_name, option, order);
                if (return_value == 0){
                    logging(command, "success", "Student grades sorted");
                    free(command);
                    exit(0);
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Read operation is not successful\n");
                    logging(command, "fail", "Read operation is not successful");
                    free(command);
                    exit(1);
                }
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
        }


        // Display Student Grades:
        // The user should be able to display all student grades stored in the file. 
        // The program should display the student name and grade for each student. Also display content page by page and first 5 entries.
        // showAll <file_name>
        else if(strncmp(string, "showAll", 7) == 0){
            
            char *command  = strdup(string);

            char ** tokens = (char **)malloc(100 * sizeof(char *));
            int num_of_tokens = divide_string(string, tokens);

            if (num_of_tokens != 2){
                printf("Incorrect number of arguments! Expected: 1\n");
                free(tokens);
                logging(command, "fail", "Incorrect number of arguments! Expected: 1");
                free(command);
                continue;
            }

            char *file_name = tokens[1];

            free(tokens);

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }

            else if (pid == 0){
                int return_value = show_all(file_name);
                if (return_value == 0){
                    logging(command, "success", "Student grades displayed");
                    free(command);
                    exit(0);
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Read operation is not successful\n");
                    logging(command, "fail", "Read operation is not successful");
                    free(command);
                    exit(1);
                }
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);

        }

        // listGrades <file_name>
        // The user should be able to list first 5 entries of the file.
        else if(strncmp(string, "listGrades", 10) == 0){
            
            char *command  = strdup(string);

            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);


            if (num_of_tokens != 2){
                printf("Incorrect number of arguments! Expected: 1\n");
                free(tokens);
                logging(command, "fail", "Incorrect number of arguments! Expected: 1");
                free(command);
                continue;
            }

            char *file_name = tokens[1];
            free(tokens);

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }

            else if (pid == 0){
                int return_value = list_grades(file_name);
                if (return_value == 0){
                    logging(command, "success", "Student grades listed");
                    free(command);
                    exit(0);
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Read operation is not successful\n");
                    logging(command, "fail", "Read operation is not successful");
                    free(command);
                    exit(1);
                }
                else if (return_value == -3){
                    printf("There are less than 5 entries in the file\n");
                    logging(command, "fail", "There are less than 5 entries in the file");
                    free(command);
                    exit(1);
                }
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
        }

        // listSome <num_of_entries> <page_number> <file_name>
        // The user should be able to list the entries of the file.
        else if(strncmp(string, "listSome", 8) == 0){

            char *command  = strdup(string); 

            char **tokens = (char **)malloc(100 * sizeof(char *));

            int num_of_tokens = divide_string(string, tokens);

            if (num_of_tokens != 4){
                printf("Incorrect number of arguments! Expected: 3\n");
                free(tokens);
                logging(command, "fail", "Incorrect number of arguments! Expected: 3");
                free(command);
                continue;
            }

            int num_of_entries = atoi(tokens[2]);
            int page_number = atoi(tokens[3]);
            char *file_name = tokens[1];

            free(tokens);

            pid_t pid = fork();

            if (pid < 0){
                perror("fork");
                logging(command, "fail", "Fork failed");
                free(command);
                continue;
            }

            else if (pid == 0){
                int return_value = list_some(file_name, num_of_entries, page_number);
                if (return_value == 0){
                    logging(command, "success", "Student grades listed");
                    free(command);
                    exit(0);
                }
                else if (return_value == -1){
                    printf("File cannot be opened\n");
                    logging(command, "fail", "File cannot be opened");
                    free(command);
                    exit(1);
                }
                else if (return_value == -2){
                    printf("Read operation is not successful\n");
                    logging(command, "fail", "Read operation is not successful");
                    free(command);
                    exit(1);
                }
                else if (return_value == -3){
                    printf("There are not enough entries in the file\n");
                    logging(command, "fail", "There are not enough entries in the file");
                    free(command);
                    exit(1);
                }
            }
            else{
                waitpid(-1, NULL, 0);
            }

            free(command);
        }


        // Invalid command
        else {
            printf("Invalid command\n");
            char * command = strdup(string);
            logging(command, "fail", "Invalid command");
            free(command);
        }
    }
    
    return 0;
}