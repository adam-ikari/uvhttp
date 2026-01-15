#include <stdio.h>
#include <stdint.h>

typedef struct {
    uint8_t fin : 1;
    uint8_t rsv1 : 1;
    uint8_t rsv2 : 1;
    uint8_t rsv3 : 1;
    uint8_t opcode : 4;
    uint8_t mask : 1;
    uint8_t payload_len : 7;
} test_header_t;

int main() {
    test_header_t header;
    
    printf("sizeof(test_header_t): %zu\n", sizeof(test_header_t));
    printf("sizeof(header.payload_len): %zu\n", sizeof(header.payload_len));
    
    header.payload_len = 255;
    printf("header.payload_len after setting to 255: %d\n", header.payload_len);
    
    header.payload_len = 256;
    printf("header.payload_len after setting to 256: %d\n", header.payload_len);
    
    return 0;
}