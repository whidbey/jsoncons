/// Copyright 2018 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CBOR_CBOR_ERROR_HPP
#define JSONCONS_CBOR_CBOR_ERROR_HPP

#include <system_error>
#include <jsoncons/config/jsoncons_config.hpp>

namespace jsoncons { namespace cbor {

enum class cbor_errc
{
    ok = 0,
    unexpected_eof = 1,
    source_error
};

class cbor_error_category_impl
   : public std::error_category
{
public:
    virtual const char* name() const noexcept
    {
        return "cbor";
    }
    virtual std::string message(int ev) const
    {
        switch (static_cast<cbor_errc>(ev))
        {
        case cbor_errc::unexpected_eof:
            return "Unexpected end of file";
        case cbor_errc::source_error:
            return "Source error";
       default:
            return "Unknown CBOR parser error";
        }
    }
};

inline
const std::error_category& cbor_error_category()
{
  static cbor_error_category_impl instance;
  return instance;
}

inline 
std::error_code make_error_code(cbor_errc e)
{
    return std::error_code(static_cast<int>(e),cbor_error_category());
}

#if !defined(JSONCONS_NO_DEPRECATED)
typedef cbor_errc cbor_parser_errc;
#endif

}}

namespace std {
    template<>
    struct is_error_code_enum<jsoncons::cbor::cbor_errc> : public true_type
    {
    };
}

#endif