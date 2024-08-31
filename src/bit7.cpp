/*

bit7.cpp
--------

Copyright (C) 2016, Tomislav Karastojkovic (http://www.alepho.com).

Distributed under the FreeBSD license, see the accompanying file LICENSE or
copy at http://www.freebsd.org/copyright/freebsd-license.html.

*/


#include <string>
#include <vector>
#include <boost/algorithm/string/trim.hpp>
#include <mailio/bit7.hpp>


using std::string;
using std::vector;
using boost::trim_right;


namespace mailio
{


// TODO: `codec` constructor is to satisfy compiler. Once these policies are removed, no need to set them.
bit7::bit7(string::size_type line1_policy, string::size_type lines_policy) :
    codec(line_len_policy_t::RECOMMENDED, line_len_policy_t::RECOMMENDED),
    line1_policy_(line1_policy), lines_policy_(lines_policy)
{
}


vector<string> bit7::encode(const string& text) const
{
    vector<string> enc_text;
    string line;
    string::size_type line_len = 0;
    bool is_first_line = true;
    const bool is_folding = (line1_policy_ != lines_policy_);
    const string FOLD_STR = (is_folding ? codec::SPACE_STR + codec::SPACE_STR : "");
    const string DELIMITERS = " ,;";
    string::size_type delim_pos = 0;

    auto add_new_line = [&enc_text, &line_len, &delim_pos](bool is_folding, string& line)
    {
        if (is_folding && delim_pos > 0)
        {
            enc_text.push_back(line.substr(0, delim_pos + 1));
            line = line.substr(delim_pos + 1);
            line_len -= delim_pos - 1;
            delim_pos = 0;
        }
        else
        {
            enc_text.push_back(line);
            line.clear();
            line_len = 0;
        }
    };

    for (auto ch = text.begin(); ch != text.end(); ch++)
    {
        if (DELIMITERS.find(*ch) != string::npos)
            delim_pos = line_len;

        if (is_allowed(*ch))
        {
            line += *ch;
            line_len++;
        }
        else if (*ch == '\r' && (ch + 1) != text.end() && *(ch + 1) == '\n')
        {
            line = FOLD_STR + line;
            add_new_line(is_folding, line);
            // Skip both crlf characters.
            ch++;
        }
        else
            throw codec_error("Bad character `" + string(1, *ch) + "`.");

        if (is_first_line)
        {
            if (line_len == line1_policy_)
            {
                is_first_line = false;
                add_new_line(is_folding, line);
            }
        }
        else if (line_len == lines_policy_ - FOLD_STR.length())
        {
            line = FOLD_STR + line;
            add_new_line(is_folding, line);
        }
    }
    if (!line.empty())
    {
        if (is_folding && !is_first_line)
            line = FOLD_STR + line;
        enc_text.push_back(line);
    }
    while (!enc_text.empty() && enc_text.back().empty())
        enc_text.pop_back();

    return enc_text;
}


string bit7::encode_str(const std::string& text) const
{
    std::vector<std::string> v = encode(text);
    string s;
    for (const auto& l : v)
        s += l + codec::END_OF_LINE;
    return s;
}



// TODO: Consider the first line policy.
string bit7::decode(const vector<string>& text) const
{
    string dec_text;
    for (const auto& line : text)
    {
        if (line.length() > lines_policy_)
            throw codec_error("Line policy overflow.");

        for (auto ch : line)
        {
            if (!is_allowed(ch))
                throw codec_error("Bad character `" + string(1, ch) + "`.");

            dec_text += ch;
        }
        dec_text += "\r\n";
    }
    trim_right(dec_text);

    return dec_text;
}


/*
For details see [rfc 2045, section 2.7].
*/
bool bit7::is_allowed(char ch) const
{
    if (strict_mode_)
        return (ch > NIL_CHAR && ch <= TILDE_CHAR && ch != CR_CHAR && ch != LF_CHAR);
    else
        return (ch != NIL_CHAR && ch != CR_CHAR && ch != LF_CHAR);
}


} // namespace mailio
