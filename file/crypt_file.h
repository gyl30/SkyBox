#ifndef LEAF_CRYPT_FILE_H
#define LEAF_CRYPT_FILE_H

#include <boost/system/error_code.hpp>

#include "file.h"
#include "crypt.h"
#include "sha256.h"

namespace leaf
{
struct crypt_file
{
    static void encode(leaf::reader* r, leaf::writer* w, encrypt* e, sha256* rs, sha256* ws, boost::system::error_code& ec);
    static void decode(leaf::reader* r, leaf::writer* w, decrypt* d, sha256* rs, sha256* ws, boost::system::error_code& ec);
};
}    // namespace leaf

#endif
