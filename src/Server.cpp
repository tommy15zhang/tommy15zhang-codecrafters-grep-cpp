#include <iostream>
#include <string>
#include <algorithm>





bool match_pattern(const std::string& input_line, const std::string& pattern) {
    if (pattern.length() == 1) {
        return input_line.find(pattern) != std::string::npos;
    }
    else if (pattern == "\\d"){ // we use \\d to avoid confusion with things like \n
        //match digits
        return input_line.find_first_of("0123456789") != std::string::npos;
    }
    else if (pattern == "\\w") { // word character: [A-Za-z0-9_]
        return input_line.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos;
    }
    // implement negative character groups. .substr(pos, len) (has to be before positive character group)
    else if (pattern.size() >= 3 && pattern.front() == '[' && pattern[1] == '^' && pattern.back() == ']'){
        std::string group = pattern.substr(2, pattern.size()-3);
        return input_line.find_first_not_of(group) != std::string::npos;
    }
    else if (pattern.size() >= 2 && pattern.front() == '[' && pattern.back() == ']') {
        std::string group = pattern.substr(1, pattern.size()-2); 
        return input_line.find_first_of(group) != std::string::npos;
    }
    else {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
}

int main(int argc, char* argv[]) {
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    // You can use print statements as follows for debugging, they'll be visible when running tests.
    std::cerr << "Logs from your program will appear here" << std::endl;

    if (argc != 3) {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];

    if (flag != "-E") {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    // Uncomment this block to pass the first stage
    
    std::string input_line;
    std::getline(std::cin, input_line);
    
    try {
        if (match_pattern(input_line, pattern)) {
            return 0;
        } else {
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
