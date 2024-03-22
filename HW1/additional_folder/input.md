# Example Commands:

## gtuStudentGrades
### Description:
List all of the instructions or information provided about how commands on your program.
### Command:
```bash
gtuStudentGrades
```
### Examples:
#### Valid Examples:
```bash
gtuStudentGrades
```

## gtuStudentGrades (create a file)
### Description:
Create a file with the given name
### Command:
```bash
gtuStudentGrades "<file_name>"
```
### Examples:
#### Valid Examples:
```bash
gtuStudentGrades "computer_science_students.txt"
gtuStudentGrades "2024_spring.txt"
```
#### Invalid Examples:
```bash
gtuStudentGrades "computer_science_students.txt" "aa"
```
## addStudentGrade
### Description:
Add a student grade to the file
### Command:
```bash
addStudentGrade "<file_name>" "<student_name>" "<student_grade>"
```

### Examples:
#### Valid Examples:
```bash
addStudentGrade "computer_science_students.txt" "John Smith" "BA"
addStudentGrade "2024_spring.txt" "Emily Davis" "CB"
addStudentGrade "computer_science_students.txt" "Michael Brown" "AA"
addStudentGrade "2024_spring.txt" "Jessica Taylor" "BB"
addStudentGrade "computer_science_students.txt" "Daniel Miller" "CC"
addStudentGrade "2024_spring.txt" "Sophia Wilson" "DC"
addStudentGrade "computer_science_students.txt" "David Jones" "DD"
addStudentGrade "2024_spring.txt" "Olivia Garcia" "FD"
addStudentGrade "computer_science_students.txt" "James Martin" "FF"
addStudentGrade "2024_spring.txt" "Isabella Rodriguez" "AA"
addStudentGrade "computer_science_students.txt" "Emma Martinez" "AB"
addStudentGrade "2024_spring.txt" "Lucas Hernandez" "AC"
addStudentGrade "computer_science_students.txt" "Mia Lopez" "AD"
addStudentGrade "2024_spring.txt" "Alexander Gonzalez" "AE"
addStudentGrade "computer_science_students.txt" "Ava Anderson" "AF"
addStudentGrade "2024_spring.txt" "Ethan Young" "AG"
addStudentGrade "computer_science_students.txt" "Charlotte Hall" "AH"
addStudentGrade "2024_spring.txt" "Jacob Allen" "AI"
addStudentGrade "computer_science_students.txt" "Amelia Wright" "AJ"
addStudentGrade "2024_spring.txt" "Michael King" "AK"
addStudentGrade "computer_science_students.txt" "Sofia Scott" "AL"
addStudentGrade "2024_spring.txt" "Benjamin Green" "BA"
addStudentGrade "computer_science_students.txt" "Chloe Adams" "BB"
addStudentGrade "2024_spring.txt" "William Baker" "BC"
addStudentGrade "computer_science_students.txt" "Lily Turner" "BD"
addStudentGrade "2024_spring.txt" "Zoe Phillips" "BE"
addStudentGrade "computer_science_students.txt" "Alexander White" "BF"
addStudentGrade "2024_spring.txt" "Madison Harris" "BG"
addStudentGrade "computer_science_students.txt" "Elijah Lewis" "BH"
addStudentGrade "2024_spring.txt" "Grace Walker" "BI"
addStudentGrade "computer_science_students.txt" "James Robinson" "BJ"
addStudentGrade "2024_spring.txt" "Victoria Clark" "CA"
addStudentGrade "computer_science_students.txt" "Mason Hill" "CB"
addStudentGrade "2024_spring.txt" "Ella Perez" "CC"
addStudentGrade "computer_science_students.txt" "Maxwell Nelson" "CD"
addStudentGrade "2024_spring.txt" "Nora Carter" "CE"
addStudentGrade "computer_science_students.txt" "Ryan Mitchell" "CF"
addStudentGrade "2024_spring.txt" "Camila Roberts" "CG"
addStudentGrade "computer_science_students.txt" "Lucas Moore" "CH"
addStudentGrade "2024_spring.txt" "Lauren Jackson" "CI"
addStudentGrade "computer_science_students.txt" "Isaac Lee" "CJ"
addStudentGrade "2024_spring.txt" "Liam Thompson" "CK"
addStudentGrade "computer_science_students.txt" "Emma Anderson" "CL"
addStudentGrade "2024_spring.txt" "Oliver Martinez" "DA"
addStudentGrade "computer_science_students.txt" "Avery Thomas" "DB"
addStudentGrade "2024_spring.txt" "Brooklyn White" "DC"
addStudentGrade "computer_science_students.txt" "Sebastian Taylor" "DD"
addStudentGrade "2024_spring.txt" "Zoe Brooks" "DE"
```

#### Invalid Examples:
```bash
addStudentGrade "computer_science_students.txt" "John Doe" "AA" "aa"

addStudentGrade "computer_science_students.txt"

addStudentGrade "computer_science_students.txt" "John Doe"

addStudentGrade "computer_science_students.txt" "AA"

addStudentGrade 
```

## searchStudent
### Description:
Search for a student in the file
### Command:
```bash
searchStudent "<file_name>" "<student_name>"
```

### Examples:
#### Valid Examples:
```bash
searchStudent "computer_science_students.txt" "John Smith"
searchStudent "2024_spring.txt" "Emily Davis"
searchStudent "computer_science_students.txt" "Michael Brown"
searchStudent "2024_spring.txt" "Jessica Taylor"
searchStudent "computer_science_students.txt" "Daniel Miller"
```

#### Valid but not found Examples:
```bash
searchStudent "computer_science_students.txt" "Unknown Student"
searchStudent "2024_spring.txt" "Unknown Student"
```

#### Invalid Examples:
```bash
searchStudent "computer_science_students.txt" "John Doe" "AA"
searchStudent "computer_science_students.txt"
```
## sortAll
### Description:
Sort all the students in the file and print them

### Command:
```bash
sortAll "<file_name>" "<name|grade>" "<a|d>"
```
a: Ascending
d: Descending

### Examples:
#### Valid Examples:
```bash
sortAll "computer_science_students.txt" "name" "a"
sortAll "computer_science_students.txt" "name" "d"
sortAll "computer_science_students.txt" "grade" "a"
sortAll "computer_science_students.txt" "grade" "d"
sortAll "2024_spring.txt" "grade" "a"
sortAll "2024_spring.txt" "grade" "d"
sortAll "2024_spring.txt" "name" "a"
sortAll "2024_spring.txt" "name" "d"
```

#### Invalid Examples:
```bash
sortAll "computer_science_students.txt" "name" "a" "d"
sortAll "computer_science_students.txt" "name" "aa"
sortAll "computer_science_students.txt" "name"
sortAll "computer_science_students.txt" "a"
sortAll "computer_science_students.txt"
sortAll 
```

## showAll
### Description:
Show all the students in the file

### Command:
```bash
showAll "<file_name>"
```

### Examples:
#### Valid Examples:
```bash
showAll "computer_science_students.txt"
showAll "2024_spring.txt"
```

#### Invalid Examples:
```bash
showAll "computer_science_students.txt" "argument2" 
showAll 
```

## listGrades

### Description:
List first 5 entries of the file

### Command:
```bash
listGrades "<file_name>"
```

### Examples:
#### Valid Examples:
```bash
listGrades "computer_science_students.txt"
listGrades "2024_spring.txt"
```

#### Valid but not found Examples:


```bash
listGrades "small.txt" # if the file is containing less than 5 entries
```


#### Invalid Examples:
```bash
listGrades "computer_science_students.txt" "argument2"
listGrades 
```

## listSome
### Description:
List some entries of the file with the given number of entries and page number

### Command:
```bash
listSome "<file_name>" "<num_of_entries>" "page_number"
```

### Examples:
#### Valid Examples:
```bash
listSome "computer_science_students.txt" "5" "1"
listSome "2024_spring.txt" "3" "2"
```

#### Valid but not found Examples:
```bash
listSome "small.txt" "10" "5" # if there is no 5th page or there is no 10 entries on page 5
```

#### Invalid Examples:
```bash
listSome "computer_science_students.txt" "5" "1" "argument4"
listSome "computer_science_students.txt" "5"
listSome "computer_science_students.txt"
listSome 
```

## exit
### Description:
Exit the program

### Command:
```bash
exit
```


## Invalid Command
### Description:
If the command is not satisfied with the given commands, it will be invalid.

### Examples:
```bash
gtu
```
