// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://sourceforge.net/projects/jsoncons/files/ for latest version
// See https://sourceforge.net/p/jsoncons/wiki/Home/ for documentation.

#ifndef JSONCONS_EXT_CSV_CSV_SERIALIZER_HPP
#define JSONCONS_EXT_CSV_CSV_SERIALIZER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <ostream>
#include <cstdlib>
#include <map>
#include "jsoncons/jsoncons_config.hpp"
#include "jsoncons/output_format.hpp"
#include "jsoncons/json2.hpp"
#include "jsoncons/json_char_traits.hpp"
#include "jsoncons/json_output_handler.hpp"
#include <limits> // std::numeric_limits

namespace jsoncons_ext { namespace csv {

template <class C>
struct csv_char_traits
{
};

template <>
struct csv_char_traits<char>
{
    static bool contains_char(const std::string& s, char c)
    {
        size_t pos = s.find_first_of(c);
        return pos == std::string::npos ? false : true;
    }

    static const std::string all_literal() {return "all";};

    static const std::string minimal_literal() {return "minimal";};

    static const std::string none_literal() {return "none";};

    static const std::string nonnumeric_literal() {return "nonumeric";};
};

template <>
struct csv_char_traits<wchar_t>
{
    static bool contains_char(const std::wstring& s, wchar_t c)
    {
        size_t pos = s.find_first_of(c);
        return pos == std::string::npos ? false : true;
    }

    static const std::wstring all_literal() {return L"all";};

    static const std::wstring minimal_literal() {return L"minimal";};

    static const std::wstring none_literal() {return L"none";};

    static const std::wstring nonnumeric_literal() {return L"nonumeric";};
};

template <class C>
void escape_string(const std::basic_string<C>& s, 
                   C quote_char, C quote_escape_char,
                   std::basic_ostream<C>& os)
{
    typename std::basic_string<C>::const_iterator begin = s.begin();
    typename std::basic_string<C>::const_iterator end = s.end();
    for (typename std::basic_string<C>::const_iterator it = begin; it != end; ++it)
    {
        C c = *it;
        if (c == quote_char)
        {
            os << quote_escape_char << quote_char;
        }
        else
        {
            os << c;
        }
    }
}

template<class C>
class basic_csv_serializer : public jsoncons::basic_json_output_handler<C>
{
    struct stack_item
    {
        stack_item(bool is_object)
           : is_object_(is_object), count_(0), skip_(false)
        {
        }
        bool is_object() const
        {
            return is_object_;
        }

        bool is_object_;
        size_t count_;
        bool skip_;
    };
    enum quote_style_enum{quote_all,quote_minimal,quote_none,quote_nonnumeric};
public:
    basic_csv_serializer(std::basic_ostream<C>& os)
       : os_(os), line_delimiter_("\n"), field_delimiter_(','), quote_char_('\"'), quote_escape_char_('\"'), quote_style_(quote_minimal)
    {
    }

    basic_csv_serializer(std::basic_ostream<C>& os,
                         const jsoncons::basic_json<C>& params)
       : os_(os), field_delimiter_(','), quote_char_('\"'), quote_escape_char_('\"')
    {

        line_delimiter_ = params.get("line_delimiter","\n").as_string();
        field_delimiter_ = params.get("field_delimiter",",").as_char();
        quote_char_ = params.get("quote_char","\"").as_char();
        quote_escape_char_ = params.get("quote_escape_char","\"").as_char();
        std::basic_string<C> quote_style = params.get("quote_style","minimal").as_string();
        if (quote_style == csv_char_traits<C>::all_literal())
        {
            quote_style_ = quote_all;
        }
        else if (quote_style == csv_char_traits<C>::minimal_literal())
        {
            quote_style_ = quote_minimal;
        }
        else if (quote_style == csv_char_traits<C>::none_literal())
        {
            quote_style_ = quote_none;
        }
        else if (quote_style == csv_char_traits<C>::nonnumeric_literal())
        {
            quote_style_ = quote_nonnumeric;
        }
        else 
        {
            JSONCONS_THROW_EXCEPTION("Unrecognized quote style.");
        }
    }

    ~basic_csv_serializer()
    {
    }

    virtual void begin_json()
    {
    }

    virtual void end_json()
    {
    }

    virtual void begin_object()
    {
        stack_.push_back(stack_item(true));
    }

    virtual void end_object()
    {
        if (stack_.size() == 2)
        {
            os_ << line_delimiter_;
            if (stack_[0].count_ == 0)
            {
                os_ << header_os_.str() << line_delimiter_;
            }
        }
        stack_.pop_back();

        end_value();
    }

    virtual void begin_array()
    {
        stack_.push_back(stack_item(false));
    }

    virtual void end_array()
    {
        if (stack_.size() == 2)
        {
            os_ << line_delimiter_;
        }
        stack_.pop_back();

        end_value();
    }

    virtual void name(const std::basic_string<C>& name)
    {
        if (stack_.size() == 2)
        {
            if (stack_[0].count_ == 0)
            {
                if (stack_.back().count_ > 0)
                {
                    os_.put(field_delimiter_);
                }
                bool quote = false;
                if (quote_style_ == quote_all || quote_style_ == quote_nonnumeric ||
                    (quote_style_ == quote_minimal && csv_char_traits<C>::contains_char(name,field_delimiter_)))
                {
                    quote = true;
                    os_.put(quote_char_);
                }
                jsoncons_ext::csv::escape_string<C>(name, quote_char_, quote_escape_char_, os_);
                if (quote)
                {
                    os_.put(quote_char_);
                }
                header_[name] = stack_.back().count_;
            }
            else
            {
                typename std::map<std::basic_string<C>,size_t>::iterator it = header_.find(name);
                if (it == header_.end())
                {
                    stack_.back().skip_ = true;
                    //std::cout << " Not found ";
                }
                else
                {
                    stack_.back().skip_ = false;
                    while (stack_.back().count_ < it->second)
                    {
                        os_.put(field_delimiter_);
                        ++stack_.back().count_;
                    }
                //    std::cout << " (" << it->value << " " << stack_.back().count_ << ") ";
                }
            }
        }
    }

    virtual void value(const std::basic_string<C>& val)
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                value(val,header_os_);
            }
            else
            {
                value(val,os_);
            }
        }
    }

    virtual void value(double val)
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                value(val,header_os_);
            }
            else
            {
                value(val,os_);
            }
        }
    }

    virtual void value(long long val)
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                value(val,header_os_);
            }
            else
            {
                value(val,os_);
            }
        }
    }

    virtual void value(unsigned long long val)
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                value(val,header_os_);
            }
            else
            {
                value(val,os_);
            }
        }
    }

    virtual void value(bool val)
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                value(val,header_os_);
            }
            else
            {
                value(val,os_);
            }
        }
    }

    virtual void null_value()
    {
        if (stack_.size() == 2 && !stack_.back().skip_)
        {
            if (stack_.back().is_object() && stack_[0].count_ == 0)
            {
                null_value(header_os_);
            }
            else
            {
                null_value(os_);
            }
        }
    }
private:

    virtual void value(const std::basic_string<C>& val, std::basic_ostream<C>& os)
    {
        begin_value(os);

        bool quote = false;
        if (quote_style_ == quote_all || quote_style_ == quote_nonnumeric ||
            (quote_style_ == quote_minimal && csv_char_traits<C>::contains_char(val,field_delimiter_)))
        {
            quote = true;
            os.put(quote_char_);
        }
        jsoncons_ext::csv::escape_string<C>(val, quote_char_, quote_escape_char_, os);
        if (quote)
        {
            os.put(quote_char_);
        }

        end_value();
    }

    virtual void value(double val, std::basic_ostream<C>& os)
    {
        begin_value(os);

        if (jsoncons::is_nan(val) && format_.replace_nan())
        {
            os  << format_.nan_replacement();
        }
        else if (jsoncons::is_pos_inf(val) && format_.replace_pos_inf())
        {
            os  << format_.pos_inf_replacement();
        }
        else if (jsoncons::is_neg_inf(val) && format_.replace_neg_inf())
        {
            os  << format_.neg_inf_replacement();
        }
        else if (format_.floatfield() != 0)
        {
            std::basic_ostringstream<C> ss;
            ss.imbue(std::locale::classic());
            ss.setf(format_.floatfield(), std::ios::floatfield);
            ss << std::showpoint << std::setprecision(format_.precision()) << val;
            os << ss.str();
        }
        else 
        {
            std::basic_string<C> buf = jsoncons::double_to_string<C>(val,format_.precision());
            os << buf;
        }

        end_value();
        
    }

    virtual void value(long long val, std::basic_ostream<C>& os)
    {
        begin_value(os);

        os  << val;

        end_value();
    }

    virtual void value(unsigned long long val, std::basic_ostream<C>& os)
    {
        begin_value(os);

        os  << val;

        end_value();
    }

    virtual void value(bool val, std::basic_ostream<C>& os)
    {
        begin_value(os);

        os << (val ? jsoncons::json_char_traits<C>::true_literal() :  jsoncons::json_char_traits<C>::false_literal());

        end_value();
    }

    virtual void null_value(std::basic_ostream<C>& os)
    {
        begin_value(os);

        os << jsoncons::json_char_traits<C>::null_literal();

        end_value();
        
    }

    void begin_value(std::basic_ostream<C>& os)
    {
        if (!stack_.empty())
        {
            if (stack_.back().count_ > 0)
            {
                os.put(field_delimiter_);
            }
        }
    }

    void end_value()
    {
        if (!stack_.empty())
        {
            ++stack_.back().count_;
        }
    }

    std::basic_ostream<C>& os_;
    jsoncons::basic_output_format<C> format_;
    std::vector<stack_item> stack_;
    std::streamsize original_precision_;
    std::ios_base::fmtflags original_format_flags_;
    C quote_char_;
    C quote_escape_char_;
    C field_delimiter_;
    quote_style_enum quote_style_;
    std::basic_ostringstream<C> header_os_;
    std::basic_string<C> line_delimiter_;
    std::map<std::basic_string<C>,size_t> header_;
};

typedef basic_csv_serializer<char> csv_serializer;

}}

#endif
