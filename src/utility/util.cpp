#include <algorithm>

#include "util.h"

using std::string;
using std::vector;

vector<string> Util::split(const string & str, const string & sep)
{
    string s = trim(str);

    vector<string> tokens;
    for (size_t idx = s.find(sep); idx != string::npos; idx = s.find(sep)) {
        string token = s.substr(0, idx);

        if (!token.empty()) {
            tokens.push_back(token);
        }
        s.erase(0, idx + sep.size());
    }

    tokens.push_back(s);
    return tokens;
}

string Util::trim(const string & str)
{
    auto lambda = [](int c) -> bool { return std::isspace(c); };

    auto f =std::find_if_not(str.begin(), str.end(),  lambda);
    auto b =std::find_if_not(str.rbegin(),str.rend(), lambda).base();

    return ((b <= f) ? string() : string(f, b));
}

string Util::join(const vector<string> & pieces, const string & sep)
{
    string output;

    std::for_each(pieces.begin(), pieces.end(),
        [&output, &sep] (const string & str) -> void {
            if (!output.empty()) {
                output += sep;
            }
            output += str;
        });
    return output;
}
