#include "value.h"
#include "parse.h"
#include "testing.h"

/*
Todo:
- move operators into friend functions
- enhance Value type
- true lazy parsing option
- test speed on core i5
- check exceptions texts
- try to get rid of exceptions, implement integer status codes and test speed of the result
- exceptions that highlight invalid characters
- check conformance to RFC
*/
/*
 Design choices:
 - Correct behaviour for every corner case, exception with informative text
   for every error
 - API should be as comfortable to use as possible. There should be
   no unexpected behaviour or weird corner cases. All reasonable use cases
   should be easy to manage
 - Only after the API is both correct and comfortable to use, performance
   should be improved to the best possible level

 - output encoding is utf-8
 - underlying number types are int64_t and double. Double and int are never
   silently converted. Out of range int produces error. Doubles are handled
   with greatest possible precision, but rounding due to double inherent
   precision limits is acceptable and does not produce an error.
*/


int main(int argc, char *argv[]) {
    json::test();
    getchar();
    return 0;
}
