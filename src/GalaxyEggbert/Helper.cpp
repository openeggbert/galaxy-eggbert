//
// Created by robertvokac on 5/28/25.
//

#include "GalaxyEggbert/Helper.hpp"
namespace GalaxyEggbert {
    std::string Helper::formatString(
        const std::string& format,
        const std::vector<std::string>& args
    ) {
        std::string result = format;

        for (size_t i = 0; i < args.size(); ++i) {
            std::string placeholder = "{" + std::to_string(i) + "}";

            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos) {
                result.replace(pos, placeholder.length(), args[i]);
                pos += args[i].length();
            }
        }

        return result;
    }

}
