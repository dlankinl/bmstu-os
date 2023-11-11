#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    double x;
	double eps;
	double a;
	double s = 0.0;
	int i = 1;

	printf("Задайте x от -1.0 до 1.0: ");
	if (scanf("%lf", &x) != 1 || fabs(x) > 1.0)
	{
		printf("Ошибка ввода: значение х по модулю должно быть в интервале от 0.0 до 1.0\n");
		return EXIT_FAILURE;
	}
	printf("Задайте точность от 0.0 до 1.0: ");
	if (scanf("%lf", &eps) != 1 || eps <= 0.0 || eps > 1.0) {
		printf("Ошибка ввода: значение эпсилон должно быть в интервале от 0.0 до 1.0\n");
		return EXIT_FAILURE;		
	}

	a = x;
	s += a;

	while (fabs(a) >= eps)
	{
		a *= (-i * x * x) / (i + 2);
		s += a;
		i += 2;
	}

	printf("%.6lf\n", s);

    return 0;
}

// процесс блокирован в ожидании завершения ввода с клавиатуры