//
// Created by robertvokac on 5/28/25.
//

#ifndef HELPER_H
#define HELPER_H
#include <regex>
#include <string>
#define TO_STRING(a) std::to_string(a)
#define STRING_VECTOR(items) std::vector<string>{items}

namespace GalaxyEggbert
{
    class Helper
    {
    public:
        static std::string formatString(const std::string& format, const std::vector<std::string>& args);
    };
}


#endif //HELPER_H
