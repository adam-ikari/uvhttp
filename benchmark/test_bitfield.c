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
    /* 不能对位域使用 sizeof，因为位域不占用完整的字节 */
    
    header.payload_len = 127;  /* 7 位位域的最大值 */
    printf("header.payload_len after setting to 127: %d\n", header.payload_len);
    
    header.payload_len = 64;
    printf("header.payload_len after setting to 64: %d\n", header.payload_len);
    
    return 0;
}