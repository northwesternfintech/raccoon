#include "common.hpp"
#include "git.h"
#include "web/web.hpp"

#include <argparse/argparse.hpp>
#include <quill/LogLevel.h>

#include <iostream>
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

    log_i(main, "Built from {} on {}", git_Describe(), git_Branch());
    log_d(main, "Commit: \"{}\" at {}", git_CommitSubject(), git_CommitDate());
    log_d(main, "Author: {} <{}>", git_AuthorName(), git_AuthorEmail());

    if (git_AnyUncommittedChanges())
        log_w(main, "Built from dirty commit!");
}

int
main(int argc, const char** argv)
{
    // Parse args
    auto [verbosity] = process_arguments(argc, argv);

    // Start logging
    auto log_level = static_cast<uint8_t>(quill::LogLevel::Debug);

    if (verbosity <= log_level)
        log_level -= verbosity;
    else // protect from underflow
        log_level = 0;

    raccoon::logging::init(static_cast<quill::LogLevel>(log_level));

    // Print information about the build
    log_build_info();
    log_i(libcurl, "{}", curl_version());

    // Create websocket
    auto data_cb = [](raccoon::web::WebSocketConnection* conn,
                      std::vector<uint8_t> data) {
        // Read data
        std::string string_data(data.begin(), data.end());
        string_data.append("\0");

        log_d(main, "Data: {}", string_data);
        if (string_data == "deadbeef") {
            std::string str =
                R"({"type":"subscribe","channels":[{"name":"level2","product_ids":["ETH-USD"]}]})";
            std::vector<uint8_t> bytes(str.begin(), str.end());
            conn->send(bytes);
        }
    };

    raccoon::web::WebSocketConnection conn("ws://localhost:8675", data_cb);
    conn.open();

    return 0;
}
