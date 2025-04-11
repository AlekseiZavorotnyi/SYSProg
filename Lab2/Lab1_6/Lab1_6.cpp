#include <stdio.h>
#include <ctype.h>
#include <math.h>

typedef double(*callback)(double);

int ValidateNumEpsPrint(const char* argv) {
    int dotCnt = 0;
    int cnt = 0;
    if (argv[0] == '-') {
        argv++;
    }
    while (*argv != '\0') {
        if (!isdigit(*argv) and (*argv != '.')) {
            printf("Not a num\n");
            return -1;
        }
        if (*argv == '.') {
            dotCnt++;
            argv++;
        } else {
            argv++;
            cnt++;
        }
    }
    if (dotCnt > 1) {
        printf("Not a num\n");
        return -1;
    }
    if (cnt > 9) {
        printf("Num too large\n");
        return -1;
    }
    return 1;
}

int ValidateEpsPrint(double eps) {
    if (eps < 0) {
        printf("Negative number\n");
        return -1;
    }
    if (!(eps > 0)) {
        printf("Eps can not be a zero\n");
        return -1;
    }
    if (eps > 0.2) {
        printf("Num too big\n");
        return -1;
    }
    return 1;
}

double Integral(const double eps, const double a, const double b, const callback func) {
    double sm = 0, x = a;
    while (x <= b) {
        sm += func(x) * eps;
        x += eps;
    }
    return sm;
}

double funcForA (double x) {
    double res = log(1 + x) / x;
    return res;
}

double funcForB (double x) {
    const double e = 2.7182818284;
    double res = pow(e, -(x * x / 2));
    return res;
}

double funcForC (double x) {
    double res = log((1.0 / (1 - x)));
    return res;
}

double funcForD (double x) {
    double res = pow(x, x);
    return res;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Wrong number of args\n");
        return -1;
    }

    if (ValidateNumEpsPrint(argv[1]) != 1) {
        return -1;
    }

    const double eps = atof(argv[1]);
    if (ValidateEpsPrint(eps) != 1){
        return -1;
    }
    callback functions[4] = {funcForA, funcForB, funcForC, funcForD};
    for (int i = 0; i < 4; i++) {
        double result = Integral(eps, eps, 1, functions[i]);
        printf("Result for %d: %f\n", i + 1, result);
    }
    return 0;
}