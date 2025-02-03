#include <stdio.h>
#include <unistd.h>

int main() {
    for (int i = 1; i <= 10; i++) {
        printf("%d\n", i);
        fflush(stdout);  // Flush the buffer to display the output immediately
        sleep(1);  // Delay for 1 second
    }
    return 0;
}
