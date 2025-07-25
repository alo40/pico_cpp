#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    char name[50];
    int age;
    int currentYear;

    // Get the current year
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    currentYear = tm.tm_year + 1900;

    // Ask for user input
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);

    // Remove newline from name input
    name[strcspn(name, "\n")] = '\0';

    printf("Enter your age: ");
    scanf("%d", &age);

    // Calculate the year the user will turn 100
    int yearAt100 = currentYear + (100 - age);

    printf("Hello, %s! You will turn 100 years old in the year %d.\n", name, yearAt100);

    // Extra: Countdown from current age to 0
    printf("Counting down your age:\n");
    for (int i = age; i >= 0; i -= 10) {
        printf("%d...\n", i);
    }

    return 0;
}
