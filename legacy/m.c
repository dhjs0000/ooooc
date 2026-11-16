#include "ooooc.h"

namespace(mynamespace)

int N_MED(11, mynamespace, 3, add, ii)(int a, int b) {
    return a + b;
}

endnamespace(mynamespace)

#include <stdio.h>
int main() {
    printf("%d\n", N_MED(11, mynamespace, 3, add, ii)(2, 3));
}