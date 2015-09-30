#include <stdio.h>
#include "datapacket.h"

int main(void)
{
    printf("starting the test\n");
    datapacket *dp = datapacket_create(0x89abcdef);
    printf("dp initialized with id=0x89abcdef\n");

    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);
    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);
    printf("set_bool: false\n");
    datapacket_set_bool(dp, false);
    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);

    printf("set_string: penis\n");
    datapacket_set_string(dp, "penis");

    printf("set_bool: true\n");
    datapacket_set_bool(dp, true);

    printf("\ndp data set\n\n");

    datapacket_dump(dp);

    printf("\nreading directly from packet.\n");
    datapacket_reset(dp);
    printf("get_int: %02X\n", datapacket_get_int(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("get_string: %s\n", datapacket_get_string(dp));
    printf("get_bool: %d\n", datapacket_get_bool(dp));
    printf("\ndone.\n");
}
