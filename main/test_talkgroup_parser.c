#include "talkgroup_model.h"
#include <assert.h>
int main(void) {
    assert(bm_parse_spoken_number("Four five four zero one") == 45401);
    assert(bm_parse_spoken_number("Ninety-one") == 91);
    assert(bm_parse_spoken_number("四五四零一") == 45401);
    assert(bm_parse_spoken_number("九十一") == 91);
    assert(bm_parse_spoken_number("四六零二零") == 46020);
    return 0;
}
