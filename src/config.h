// logging
#define LOG_BACKTRACE_SIZE 10

#define LOG_DIR            "logs"
#define LOG_FILE           (LOG_DIR "/app.log")

#define LOG_FILE_SIZE      (1024 * 1024 / 2) // 512 KB
#define LOG_BACKUP_COUNT   5

// Web
#define FD_LIMIT_PER_THREAD 100

// Helper macros
#define UNUSED(x) (void)(x)
