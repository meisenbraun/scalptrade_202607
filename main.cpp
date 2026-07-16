#include <iostream>
#include <memory>
#include "application.h"
#include "eventloop.h"
#include "exit_status.h"
#include "programargs.h"
#include "tcpconnection.h"

void debug_args(int argc, char** argv)
{
    for (int i = 0; i < argc; ++i)
    {
        std::string arg = argv[i];
        std::cout << arg << "\n";
    }
}

int main(int argc, char** argv)
{
    bool retval = exit_status_success;

    Application app(argc, argv);

    app.init();
    app.run();

    return retval;
}
