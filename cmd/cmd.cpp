#include "application.h"

int main(int argc, char *argv[])
{
    leaf::application app(argc, argv);
    return app.exec();
}
