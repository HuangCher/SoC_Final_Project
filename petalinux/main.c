#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bloom_driver.h"

#define BASE_ADDR 0x44A00000u

int main() {
    bloom_init(BASE_ADDR);

    // cli interface
    char cmd[20];
    uint32_t key;

    printf("Bloom Filter CLI\n");
    printf("Commands: insert <key>, query <key>, exit\n");

    while (1) {
        // prompt user for command
        printf("> ");
        scanf("%s", cmd);

        // handle commands
        if (strcmp(cmd, "insert") == 0) {
            scanf("%u", &key);
            bloom_insert(key);
            printf("Inserted %u\n", key);
        }

        else if (strcmp(cmd, "query") == 0) {
            scanf("%u", &key);
            int res = bloom_query(key);

            if (res)
                printf("MAYBE present\n");
            else
                printf("NOT present\n");
        }

        else if (strcmp(cmd, "exit") == 0) {
            break;
        }

        else {
            printf("Unknown command\n");
        }
    }

    return 0;
}