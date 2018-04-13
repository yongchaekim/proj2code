#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <math.h>
#define __NR_rotlock_read       381
#define __NR_rotunlock_read     385

void computeprime(int);

int main(int argc, char *argv[])
{
        if(argc == 1)
        {
                fputs("Error! please correct your options\n", stderr);
                exit(1);
        }

        int integer = atoi(argv[1]);
        int range = 60;
        int degree = 30;
        int retval;

        if(degree < 0 || degree > 180)
        {
                printf("error");
                exit(1);
        }

        FILE *file = fopen("Integer.txt", "r");

        if(file == NULL)
        {
                printf("File is not working");
                exit(1);
        }

        while(1)
        {
                fseek(file, sizeof(int), SEEK_SET);
                syscall(__NR_rotlock_read, degree, range);
                fscanf(file, "%d", &retval);
                printf("trial %d: ", integer);
                computeprime(retval);
                printf("\n");
                syscall(__NR_rotunlock_read, degree, range);
        }
        fclose(file);
        return 0;
}
void computeprime(int n)
{
        while(n % 2 == 0)
        {
                printf("%d ", 2);
                n = n / 2;
                if(n > 2)
                printf("* ");
        }

        for(int i = 3; i <= sqrt(n); i = i + 2)
        {
                while(n % i == 0)
                {
                        printf("%d ", i);
                        n = n / i;
                        if(n > 2)
                        printf("* ");
                }
        }
        if(n > 2)
        printf("%d ", n);
}
