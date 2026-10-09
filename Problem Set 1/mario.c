#include <cs50.h>
#include <stdio.h>

void print_row(int row, int height);

int main(void)
{
    // Prompt for height, reprompt until it is between 1 and 8
    int height;
    do
    {
        height = get_int("Height: ");
    }
    while (height < 1 || height > 8);

    // Print the pyramid one row at a time
    for (int row = 1; row <= height; row++)
    {
        print_row(row, height);
    }
}

// Prints a single row: left pyramid, two-space gap, right pyramid
void print_row(int row, int height)
{
    // Leading spaces to right-align the left pyramid
    for (int space = 0; space < height - row; space++)
    {
        printf(" ");
    }

    // Left hashes
    for (int hash = 0; hash < row; hash++)
    {
        printf("#");
    }

    // Gap
    printf("  ");

    // Right hashes
    for (int hash = 0; hash < row; hash++)
    {
        printf("#");
    }

    printf("\n");
}

