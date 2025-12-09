#include "core/Application.h"

int main(int argc, char *argv[])
{
    using namespace XXMLStudio;

    // Create application instance
    Application::create(argc, argv);

    // Run the application
    int result = Application::instance()->run();

    // Cleanup
    Application::destroy();

    return result;
}
