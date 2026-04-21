#include <stdio.h>
#include <limits.h>

static inline int gui_check_horizontal(int d, int dmin, int o, int omin)
{
    return (o > 0 && (omin > 0 ? d <= dmin : 1));
}

static inline int gui_check_vertical(int d, int dmin, int o, int omin)
{
    return (omin > 0 ?
            d < dmin :
            (o > 0 ? d <= dmin : (d < dmin || (d == dmin && o > omin))));
}

typedef struct {
    int d, dmin, o, omin;
    int expected_h;
    int expected_v;
} Test;

Test tests[] = {
    /* Horizontal tests */
    { 10, 20, 0, 5, 0, 1 },
    { 10, 20, -1, 5, 0, 1 },
    { 10, 20, 5, 5, 1, 1 },
    { 20, 20, 5, 5, 1, 0 },
    { 21, 20, 5, 5, 0, 0 },
    { 10, 20, 5, 0, 1, 1 },
    { 30, 20, 5, -1, 1, 0 },

    /* Vertical tests */
    { 10, 20, 0, 0, 0, 1 },
    { 10, 20, -5, -10, 0, 1 },
    { 20, 20, -5, -10, 0, 1 },
    { 20, 20, -10, -10, 0, 0 },
    { 20, 20, -15, -10, 0, 0 },
    { 21, 20, 0, 0, 0, 0 },

    /* Initial case tests */
    { 10, 100, 5, INT_MIN, 1, 1 },
    { 10, 100, -5, INT_MIN, 0, 1 },
};

int main() {
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    int failed = 0;

    for (int i = 0; i < num_tests; i++) {
        int d = tests[i].d;
        int dmin = tests[i].dmin;
        int o = tests[i].o;
        int omin = tests[i].omin;

        int res_h = gui_check_horizontal(d, dmin, o, omin) ? 1 : 0;
        int res_v = gui_check_vertical(d, dmin, o, omin) ? 1 : 0;

        if (res_h != tests[i].expected_h || res_v != tests[i].expected_v) {
            printf("Test %d failed: d=%2d dmin=%2d o=%2d omin=%2d | H: exp=%d got=%d, V: exp=%d got=%d\n",
                   i, d, dmin, o, omin, tests[i].expected_h, res_h, tests[i].expected_v, res_v);
            failed++;
        }
    }

    if (failed == 0) {
        printf("All %d tests passed!\n", num_tests);
        return 0;
    } else {
        printf("%d tests failed!\n", failed);
        return 1;
    }
}
