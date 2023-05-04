#include <stdio.h>

int main(int argc, char const *argv[]) {
    char c1 = '\n';
    FILE *f = fopen("test.txt", "w+");
    for (int i = 0; i < 20; i++) {
        char c = i + 97;
        for (int j = 0; j < 1023; j++) {
            fwrite(&c, 1, 1, f);
        }
        fwrite(&c1, 1, 1, f);
    }
    return 0;
}
