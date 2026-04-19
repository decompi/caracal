#include <stdio.h>

int main(void) {
    double a[2] = {1.5, 2.5};
    double b[2] = {10.0, 20.0};

    int outer = 0;
    double total = 0.0;

    while (outer < 4096) {
        int inner = 0;

        while (inner < 4096) {
            total = total + (a[0] * b[0]);
            total = total + (a[1] * b[1]);
            inner = inner + 1;
        }

        outer = outer + 1;
    }

    printf("%f\n", total);
    return 0;
}