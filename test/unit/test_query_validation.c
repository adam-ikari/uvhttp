#include <stdio.h>
#include <uvhttp_validation.h>

int main() {
    const char* query = "key=value";
    int result = uvhttp_validate_query_string(query);
    printf("Validation result for '%s': %d\n", query, result);
    
    query = "key1=value1&key2=value2";
    result = uvhttp_validate_query_string(query);
    printf("Validation result for '%s': %d\n", query, result);
    
    return 0;
}