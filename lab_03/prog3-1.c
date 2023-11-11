#include <stdio.h>
#include <stdlib.h>

int main() {
    int v_len;
    printf("Задайте длину вектора, большую либо равную 0: ");
    if (scanf("%d", &v_len) != 1 || v_len < 0) {
        printf("Ошибка ввода: вы должны задать значение длины, большее или равное 0.\n");
        return 1;
    }
    printf("Заданная длина: %d\n", v_len);

    return 0;
}