#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#define __NR_rotlock_write      382
#define __NR_rotunlock_write    383

int main(int argc, char *argv[])
{
        if(argc == 1)
        {
                fputs("Error! please right your options\n", stderr);
                exit(1);
        }

        int integer = atoi(argv[1]);
        int range = 60;
        int degree = 30; // just change this  also degree is 0 <= range <= 90 and 330 <= range <= 360

        if(degree < 0 || degree > 180)
        {
                printf("error");
                exit(1);
        }

        FILE *file = fopen("Integer.txt", "w");

        if(file == NULL)
        {
                printf("File is not working");
                exit(1);
        }

        while(1)
        {
                fseek(file, sizeof(int), SEEK_SET);

                syscall(__NR_rotlock_write, degree, range);
                fprintf(file, "%d", integer);
                printf("selector: %d\n", integer);
                syscall(__NR_rotunlock_write, degree, range);

                integer = integer + 1;

        }
        fclose(file);

        return 0;
}
