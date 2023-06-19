#include "git.h"
#include "logging.hpp"

#include <iostream>
#include <string>

int
main()
{
    // Start logging
    // TODO(nino): verbosity switch
    raccoon::logging::init(quill::LogLevel::TraceL3);

    log_i(main, "Raccoon: Data Acquisition for NUFT");

    // Print information about the build
    log_i(main, "Built from {} on {}", git_Describe(), git_Branch());
    log_d(main, "Commit: \"{}\" at {}", git_CommitSubject(), git_CommitDate());
    log_d(main, "Author: {} <{}>", git_AuthorName(), git_AuthorEmail());

    if (git_AnyUncommittedChanges())
        log_w(main, "Built from dirty commit!");

    return 0;
}
