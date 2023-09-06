#include "common.hpp"
#include "git.h"
#include "storage/storage.hpp"
#include "utils/utils.hpp"
#include "web/manager.hpp"
#include "web/web.hpp"

#include <argparse/argparse.hpp>
#include <curl/curl.h>
#include <hiredis/hiredis.h>
#include <quill/LogLevel.h>
#include <uv.h>

#include <iostream>
#include <string_view>
#include <tuple>

constexpr static std::string_view COINBASE_SUBSCRIBE_STR = R"(
{
  "type": "subscribe",
  "channels": [
    {
      "name": "matches",
      "product_ids": [
        "ETH-USD"
      ]
    },
    {
      "name": "level2_batch",
      "product_ids": [
        "ETH-USD"
      ]
    }
  ]
}
)";

static std::tuple<uint8_t>
process_arguments(int argc, const char** argv)
{
    argparse::ArgumentParser program(
        "krompir", VERSION, argparse::default_arguments::help
    );

    program.add_argument("-V", "--version")
        .help("prints version information and exits")
        .action([&](const auto& /* unused */) {
            fmt::println("raccoon v{}", VERSION);
            exit(0); // NOLINT(concurrency-*)
        })
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    uint8_t verbosity = 0;
    program.add_argument("-v", "--verbose")
        .help("increase output verbosity")
        .action([&](const auto& /* unused */) { ++verbosity; })
        .append()
        .default_value(false)
        .implicit_value(true)
        .nargs(0);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        exit(1); // NOLINT(concurrency-*)
    }

    return std::make_tuple(verbosity);
}

static void
log_build_info()
{
    log_i(main, "Raccoon: Data Acquisition for NUFT");

    // Git info
    log_i(main, "Built from {} on {}", git_Describe(), git_Branch());
    log_d(main, "Commit: \"{}\" at {}", git_CommitSubject(), git_CommitDate());
    log_d(main, "Author: {} <{}>", git_AuthorName(), git_AuthorEmail());

    if (git_AnyUncommittedChanges())
        log_w(main, "Built from dirty commit!");

    // Library info
    log_i(libcurl, "{}", curl_version());
}

int
main(int argc, const char** argv)
{
    // Parse args
    auto [verbosity] = process_arguments(argc, argv);

    // Start logging and print build info
    raccoon::logging::init(verbosity);
    log_build_info();

    // Connect to redis
    namespace utils = raccoon::utils; // TEMP: redis will be refactored into own dir

    auto redis_url = utils::getenv("REDIS_URL", "127.0.0.1");
    auto redis_port = std::stoi(utils::getenv("REDIS_PORT", "6379"));

    redisContext* ctx = redisConnect(redis_url.c_str(), redis_port);

    if (ctx == nullptr || ctx->err) [[unlikely]] {
        if (ctx) {
            log_e(main, "Could not connect to Redis: {}\n", ctx->errstr);
        }
        else {
            log_e(main, "Can't allocate redis context\n");
        }
        return 1;
    }

    log_i(main, "Successfully connected to redis");

    raccoon::storage::DataProcessor prox(ctx);

    // Init curl
    if (curl_global_init(CURL_GLOBAL_ALL)) {
        log_c(main, "Could not initialize cURL");
        return 1;
    }

    // Manager testing
    raccoon::web::RequestManager session;

    // Create websocket
    auto data_cb = [&prox](auto* conn, std::vector<uint8_t> data) {
        if (memcmp(data.data(), PROXY_FIRST_MESSAGE, PROXY_FIRST_MESSAGE_LEN) == 0)
            [[unlikely]] // only the first message is like this
        {
            std::vector<uint8_t> bytes(
                COINBASE_SUBSCRIBE_STR.begin(), COINBASE_SUBSCRIBE_STR.end()
            );
            conn->send(std::move(bytes));
        }
        else [[likely]] {
            prox.process_incoming_data(data);
        }
    };

    auto ws1 = session.ws("ws://localhost:8675", data_cb);

    // Run manager
    auto err = session.run();

    if (err) [[unlikely]] {
        switch (err) {
            case raccoon::web::RequestManager::STATUS_FORCED_SHUTDOWN:
                log_c(main, "Forced shutdown, aborting!");
                abort();

            case raccoon::web::RequestManager::STATUS_GRACEFUL_SHUTDOWN:
                log_w(main, "Gracefully exiting application");
                redisFree(ctx);
                return 0;

            [[unlikely]] default:
                log_c(main, "Unknown session status {}, aborting!", err);
                abort();
        }
    }

    // Cleanup
    redisFree(ctx);

    return 0;
}
