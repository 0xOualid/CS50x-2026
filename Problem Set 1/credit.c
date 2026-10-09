#include <cs50.h>
#include <math.h>
#include <stdio.h>

int double_digit(int digit);

int main(void)
{
    // Get user input and calculate length of number
    long number = get_long("Number: ");
    int length = 1 + log10(number);

    // *** Get every second digit (starting from the 2nd last) ***
    int a = (number % 100) / 10;                  // 2nd digit
    int b = (number % 10000) / 1000;              // 4th digit
    int c = (number % 1000000) / 100000;          // 6th digit
    int d = (number % 100000000) / 10000000;      // 8th digit
    int e = (number % 10000000000) / 1000000000;  // 10th digit
    int f = (number % 1000000000000) / 100000000000; // 12th digit
    int g = 0;
    int h = 0;

    // 14th digit, only if longer than 13
    if (length > 13)
    {
        g = (number % 100000000000000) / 10000000000000;
    }

    // 16th digit, only if 16 digits long
    if (length > 15)
    {
        h = (number % 10000000000000000) / 1000000000000000;
    }

    // *** Double those digits (splitting any two-digit results) ***
    int aa = double_digit(a);
    int bb = double_digit(b);
    int cc = double_digit(c);
    int dd = double_digit(d);
    int ee = double_digit(e);
    int ff = double_digit(f);
    int gg = double_digit(g);
    int hh = double_digit(h);

    // *** Get the other digits ***
    int i = number % 10;                          // 1st digit
    int j = (number % 1000) / 100;                // 3rd digit
    int k = (number % 100000) / 10000;            // 5th digit
    int l = (number % 10000000) / 1000000;        // 7th digit
    int m = (number % 1000000000) / 100000000;    // 9th digit
    int n = (number % 100000000000) / 10000000000; // 11th digit
    int o = (number % 10000000000000) / 1000000000000; // 13th digit
    int p = 0;

    // 15th digit, only if longer than 14
    if (length > 14)
    {
        p = (number % 1000000000000000) / 100000000000000;
    }

    // *** Final stage of the algorithm ***
    int total = aa + bb + cc + dd + ee + ff + gg + hh + i + j + k + l + m + n + o + p;
    int last_digit = total % 10;

    // Length must be 13, 15 or 16
    if (length != 13 && length != 15 && length != 16)
    {
        printf("INVALID\n");
    }
    // Checksum must end in 0
    else if (last_digit != 0)
    {
        printf("INVALID\n");
    }
    // 15 digits starting with 34 or 37 is AMEX
    else if (length == 15 && p == 3 && (g == 4 || g == 7))
    {
        printf("AMEX\n");
    }
    // 13 digits starting with 4 is VISA
    else if (length == 13 && o == 4)
    {
        printf("VISA\n");
    }
    // 16 digits starting with 51-55 is MASTERCARD
    else if (length == 16 && h == 5 && p >= 1 && p <= 5)
    {
        printf("MASTERCARD\n");
    }
    // 16 digits starting with 4 is VISA
    else if (length == 16 && h == 4)
    {
        printf("VISA\n");
    }
    else
    {
        printf("INVALID\n");
    }
}

// Doubles a digit and returns the sum of the digits of the result
int double_digit(int digit)
{
    int doubled = digit * 2;

    if (doubled > 9)
    {
        return doubled - 9;
    }
    return doubled;
}

