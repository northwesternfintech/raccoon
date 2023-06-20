#include "logging.hpp"

#include "config.h"

#include <quill/Quill.h>

#include <string>

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)                \
    || defined(QUILL_NO_THREAD_NAME_SUPPORT)
#  define LOGLINE_FORMAT                                                               \
      "%(ascii_time) [%(thread:<6)] [%(logger_name:<8)] %(level_name:<8) - "           \
      "%(message) (%(fileline))"
#else
#  define LOGLINE_FORMAT                                                               \
      "%(ascii_time) [%(thread_name:<12)] [%(logger_name:<8)] %(level_name:<8) - "     \
      "%(message) (%(fileline))"
#endif

#ifdef _WIN32
#  define TZ_FORMAT ""
#else
#  define TZ_FORMAT " %z"
#endif

namespace raccoon {
namespace logging {

using namespace quill; // NOLINT(*-using-namespace)
using cc = quill::ConsoleColours;

void
init(quill::LogLevel log_level)
{
    detail::application_log_level = log_level;

#ifdef _WIN32
    // NOTE: on win32 a signal handler is needed for each new thread
    quill::init_signal_handler();
#endif

    // Create the logs directory
    if (!std::filesystem::is_directory(LOG_DIR))
        std::filesystem::create_directory(LOG_DIR);

    // Create our config object
    quill::Config cfg;

    // Set main logger name
    cfg.default_logger_name = "main";

    //
    // Initialize print handler
    //
    quill::ConsoleColours colors;
    colors.set_colour(LogLevel::TraceL3, cc::white);
    colors.set_colour(LogLevel::TraceL2, cc::white);
    colors.set_colour(LogLevel::TraceL1, cc::white);
    colors.set_colour(LogLevel::Debug, cc::cyan);
    colors.set_colour(LogLevel::Info, cc::green);

#ifdef _WIN32
    // NOLINTBEGIN(*-signed-bitwise)
    colors.set_colour(LogLevel::Warning, cc::yellow | cc::bold);
    colors.set_colour(LogLevel::Error, cc::red | cc::bold);
    colors.set_colour(LogLevel::Critical, cc::bold | cc::white | cc::on_red);
    // NOLINTEND(*-signed-bitwise)
#else
    colors.set_colour(LogLevel::Warning, cc::yellow + cc::bold);
    colors.set_colour(LogLevel::Error, cc::red + cc::bold);
    colors.set_colour(LogLevel::Critical, cc::bold + cc::white + cc::on_red);
#endif

    colors.set_colour(LogLevel::Backtrace, cc::magenta);

    auto stdout_handler = quill::stdout_handler("console", colors);
    stdout_handler->set_pattern(
        LOGLINE_FORMAT,
        "%Y-%m-%dT%T.%Qms" TZ_FORMAT // ISO 8601 but with space instead of T
    );
    stdout_handler->set_log_level(log_level);

    cfg.default_handlers.emplace_back(stdout_handler);

    //
    // Initialize rotating file handler
    //
    auto file_handler = quill::rotating_file_handler(
        LOG_FILE,
        "a",                  // append
        FilenameAppend::None, // just keep the filename
        LOG_FILE_SIZE,        // 512 KB
        LOG_BACKUP_COUNT      // 5 backups
    );

    file_handler->set_pattern(
        LOGLINE_FORMAT,
        "%Y-%m-%dT%T.%Qms" TZ_FORMAT // ISO 8601
    );
    file_handler->set_log_level(log_level);

    cfg.default_handlers.emplace_back(file_handler);

    // Send the config
    quill::configure(cfg);

    // Set backtrace and log level on the main logger
    quill::Logger* main_logger = quill::get_logger();
    main_logger->init_backtrace(LOG_BACKTRACE_SIZE, quill::LogLevel::Error);
    main_logger->set_log_level(log_level);

    // Set the thread name
    set_thread_name("MainThread");

    // Start the logging backend thread
    quill::start(true);

    LOG_INFO(main_logger, "Logging initialized!");
}

} // namespace logging
} // namespace raccoon
