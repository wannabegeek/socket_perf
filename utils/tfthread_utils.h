//
// Created by tom on 10/10/2021.
//

#ifndef TFTHREAD_UTILS_H
#define TFTHREAD_UTILS_H

#include <thread>

namespace tf {
    namespace thread {
        bool can_set_affinity() noexcept;
        int get_num_cores() noexcept;
        int get_current_core();
        void set_affinity(const std::thread::native_handle_type& thread_id, const std::initializer_list<int> cores);
        void set_affinity(std::thread &thread, const std::initializer_list<int> cores);
    }
}

#endif //TFTHREAD_UTILS_H
