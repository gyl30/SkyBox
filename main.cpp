#include <string>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdint>
#include <sodium.h>
#include <boost/filesystem.hpp>
#include "log.h"
#include "crypt.h"
#include "executors.h"

int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "Usage: " << argv[0] << " plain_file encrypt_file decrypt_file" << std::endl;
        return 0;
    }
    leaf::init_log("log.txt");
    leaf::set_log_level("debug");

    leaf::executors executors(4);
    executors.startup();

    const static std::vector<uint8_t> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                             0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
    if (boost::filesystem::exists(argv[2]))
    {
        boost::filesystem::remove(argv[2]);
    }
    if (boost::filesystem::exists(argv[3]))
    {
        boost::filesystem::remove(argv[3]);
    }

    leaf::encrypt_file e(argv[1], argv[2], key, executors.get_executor());
    e.startup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    e.shutdown();
    leaf::decrypt_file d(argv[2], argv[3], key, executors.get_executor());
    d.startup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    d.shutdown();
    //
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    executors.shutdown();
    leaf::shutdown_log();
    return 0;
}
