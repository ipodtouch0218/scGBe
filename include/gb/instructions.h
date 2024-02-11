#pragma once
#include <stdint.h>

namespace ByteRegister {
    enum ByteRegister {
        B = 0,
        C = 1,
        D = 2,
        E = 3,
        H = 4,
        L = 5,
        HL_INDIRECT = 6,
        A = 7
    };
}

namespace WordRegister {
    enum WordRegister {
        BC = 0,
        DE = 1,
        HL = 2,
        SP = 3,
        AF = 4,
    };
}

namespace Condition {
    enum Condition {
        NONZERO, ZERO, NOCARRY, CARRY
    };
}