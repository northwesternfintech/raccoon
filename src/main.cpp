#include "lib.hpp"
#include "logging.hpp"

#include <iostream>
#include <string>

int
main()
{
    auto const lib = library{};
    auto const message = "Hello from " + lib.name + "!";
    std::cout << message << '\n';

    // Start logging
    // TODO(nino): verbosity switch
    raccoon::logging::init(quill::LogLevel::TraceL3);

    log_t3(main, "This is a log trace l3 example {}", 1);
    log_t2(main, "This is a log trace l2 example {} {}", 2, 2.3f);
    log_t1(main, "This is a log trace l1 {} example", "string");
    log_d(main, "This is a log debug example {}", 4);
    log_i(main, "This is a log info example {}", 5);
    log_w(main, "This is a log warning example {}", 6);
    log_e(main, "This is a log error example {}", 7);
    log_c(main, "This is a log critical example {}", 8);

    std::array<uint32_t, 4> arr = {1, 2, 3, 4};
    log_t3(redis, "This is a log trace l3 example {}", 1);
    log_i(redis, "This is a log info example {}", arr);

    log_bt(redis, "bt1");
    log_bt(redis, "bt2");

    log_e(redis, "Test Error!");
    return 0;
}
