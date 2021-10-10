//
// Created by tom on 10/10/2021.
//

#ifndef SOCKET_PERF_OPTIMIZE_H
#define SOCKET_PERF_OPTIMIZE_H

namespace tf {
#if defined(__GNUC__)
    inline bool likely(bool x) { return __builtin_expect((x), true); }
    inline bool unlikely(bool x) { return __builtin_expect((x), false); }
#else
    inline bool likely(bool x) { return x; }
    inline bool unlikely(bool x) { return x; }
#endif
}

#endif //SOCKET_PERF_OPTIMIZE_H
