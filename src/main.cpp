#include "common.hpp"
#include "git.h"
#include "storage/storage.hpp"
#include "utils/utils.hpp"
#include "web/web.hpp"

#include <argparse/argparse.hpp>
#include <hiredis/hiredis.h>
#include <quill/LogLevel.h>

#include <iostream>
#include <string>
#include <tuple>

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
    if (ctx == nullptr || ctx->err) {
        if (ctx) {
            log_e(main, "Error: %s\n", ctx->errstr);
        }
        else {
            log_e(main, "Can't allocate redis context\n");
        }
        return 1;
    }
    log_d(main, "Successfully connected to redis");

    raccoon::storage::DataProcessor prox(ctx);

    // Create websocket
    auto data_cb = [&prox](
                       raccoon::web::WebSocketConnection* conn,
                       std::vector<uint8_t> data
                   ) {
        std::string string_data(data.begin(), data.end());
        string_data.append("\0");

        log_d(main, "Data: {}", string_data);
        if (string_data == "deadbeef") {
            std::string str =
                R"({"type":"subscribe","channels":[{"name":"matches","product_ids":["ETH-USD"]},{"name":"level2","product_ids":["ETH-USD"]}]})";
            std::vector<uint8_t> bytes(str.begin(), str.end());
            conn->send(std::move(bytes));
        }
        else {
            prox.process_incoming_data(string_data);
        }
    };

    raccoon::web::WebSocketConnection conn("ws://localhost:8675", data_cb);
    conn.open();

    // Cleanup
    redisFree(ctx);

    return 0;
}
