#include "git.h"
#include "logging.hpp"
#include "web.hpp"

#include <cstdint>
#include <cstring>

#include <iostream>
#include <string>

int
main(int argc, const char** argv)
{
    // Start logging
    // TODO(nino): verbosity switch
    raccoon::logging::init(quill::LogLevel::Debug);

    log_i(main, "Raccoon: Data Acquisition for NUFT");

    // Print information about the build
    log_i(main, "Built from {} on {}", git_Describe(), git_Branch());
    log_d(main, "Commit: \"{}\" at {}", git_CommitSubject(), git_CommitDate());
    log_d(main, "Author: {} <{}>", git_AuthorName(), git_AuthorEmail());

    if (git_AnyUncommittedChanges())
        log_w(main, "Built from dirty commit!");

    // Initialize websockets
    raccoon::web::initialize(argc, argv);

    // Create websocket
    auto data_cb = [](void* data, size_t len) {
        std::string msg(static_cast<const char*>(data), len);
        log_d(main, "Data: {}", msg);
    };
    raccoon::web::WebSocketConnection conn("libwebsockets.org", "/", data_cb);

    // Run websocket
    raccoon::web::run();

    return 0;
}
