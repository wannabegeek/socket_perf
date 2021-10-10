//
// Created by tom on 10/10/2021.
//

#ifndef TFTHREAD_UTILS_H
#define TFTHREAD_UTILS_H

#include "tflogger.h"
#include "../config.h"

#include <thread>
#include <vector>
#include <unistd.h>
#include <string.h>

#ifdef HAVE_PTHREAD_H
#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#   include <pthread.h>
#endif

namespace tf {
    namespace thread {
        bool can_set_affinity() noexcept {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
            return true;
#else
            return false;
#endif
        }

        int get_num_cores() noexcept {
            // This give the total number of addressable cores, which may include hyper-threading
            static int num_cores = std::thread::hardware_concurrency();
            return num_cores;
        }

        int get_current_core() {
            return sched_getcpu();
        }

        void set_affinity(const std::thread::native_handle_type& thread_id, const std::initializer_list<int> cores) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            const int num_cores = get_num_cores();

            for (auto &core : cores) {
                if (core < num_cores) {
                    CPU_SET(core, &cpuset);
                } else {
                    ERROR_LOG("Cannot set affinity to core " << core << " only " << num_cores << " cores available");
                }
            };
            if (pthread_setaffinity_np(thread_id, sizeof(cpu_set_t), &cpuset) == -1) {
                ERROR_LOG("Setting CPU affinity failed: " << strerror(errno));
            }
#else
            ERROR_LOG("Setting thread affinity not supported");
#endif
        }

        void set_affinity(std::thread &thread, const std::initializer_list<int> cores) {
            const std::thread::native_handle_type handle = thread.native_handle();
            tf::thread::set_affinity(handle, cores);
        }
    }
}

#endif //TFTHREAD_UTILS_H
