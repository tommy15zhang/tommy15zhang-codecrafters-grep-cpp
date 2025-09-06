#include <iostream>
#include <string>
#include <algorithm>
#include <vector>

#define DEBUG 1   // uncomment to enable debug

#ifdef DEBUG
    #define DBG_PRINT(x) do { std::cerr << x << std::endl; } while(0)
#else
    #define DBG_PRINT(x) do {} while(0)
#endif


enum class TokenType {
    Digit, // \d match 0-9
    WordChar, // \w match [A-Za-z0-9_]
    CharClass, // [abc] match a, b, or c
    NegCharClass, // [^abc] match anything but a, b, or c
    Literal // match any literal character 
};

struct Token
{
    TokenType type;
    std::string data; //for CharClass, NegCharClass and Literal
};

 // only for the pattern
std::vector<Token> tokenize(const std::string& pattern){
    std::vector<Token> toks;
    std::size_t i = 0, n = pattern.size();
    while (i < n){
        char c = pattern[i];

        //Escape: \d , \w

        if (c == '\\'){
            char next = pattern[i+1];
            if (next == 'd'){
                toks.push_back({TokenType::Digit, ""});
                i += 2;
            } else if (next == 'w'){
                toks.push_back({TokenType::WordChar, ""});
                i += 2;
            } else {
                toks.push_back({TokenType::Literal, std::string(1, next)});
                i += 2;
            }
        }
        else if (c == '['){
            std::size_t j = i + 1;
            bool is_negative = false;
            if (pattern[j] == '^') {is_negative = true; j++;}

            std::string cls;
            bool closed = false;
            for (; j < n; ++j) {
                if (pattern[j] == ']') {closed = true; break;}
                if (pattern[j] == '\\'){
                    cls.push_back(pattern[j+1]);
                    j++;
                } else {
                    cls.push_back(pattern[j]);
                }
            }
            toks.push_back({is_negative ? TokenType::NegCharClass : TokenType::CharClass, cls});
            i = j + 1;
        }
        else {
            toks.push_back({TokenType::Literal, std::string(1, c)});
            i++;
        }
    }
    return toks;
}

static inline bool is_word_char(unsigned char ch){
    return std::isalnum(ch) || ch == '_';
}

static inline bool in_class(char ch, const std::string& set){
    return set.find(ch) != std::string::npos;
}

bool match_here(const std::string& s, std::size_t start, const std::vector<Token>& toks){
    std::size_t i = start;
    std::size_t j = 0;

    while (j < toks.size()){
        if (i >= s.size()) return false;

        const Token& tok = toks[j];
        char ch = s[i];

        switch (tok.type){
            case TokenType::Digit:
                DBG_PRINT("Checking Digit against char: " << ch);
                if (!std::isdigit(static_cast<unsigned char>(ch))) return false;
                break;
            case TokenType::WordChar:
                DBG_PRINT("Checking WordChar against char: " << ch);
                if (!is_word_char(static_cast<unsigned char>(ch))) return false;
                break;
            case TokenType::CharClass:
                DBG_PRINT("Checking CharClass against char: " << ch);
                if (!in_class(ch, tok.data)) return false;
                break;
            case TokenType::NegCharClass:
                DBG_PRINT("Checking NegCharClass against char: " << ch);
                if (in_class(ch, tok.data)) return false;
                break;
            case TokenType::Literal:
                DBG_PRINT("Checking Literal against char: " << ch);
                if (ch != tok.data[0]) return false;
                break;
        }
        ++i;
        ++j;
    }
    return true;
}


bool match_pattern(const std::string& input_line, const std::string& pattern) {
    // Tokenize once
    auto toks = tokenize(pattern);
    if(toks.empty()) return true; //empty pattern matches trivalliy

    for (std::size_t pos = 0; pos < input_line.size(); pos++){
        if(match_here(input_line, pos, toks)){
            return true;
        }
    }
    return false;
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
    DBG_PRINT("Tokenizing pattern: " << pattern);

    try {
        if (match_pattern(input_line, pattern)) {
            printf("Pattern Matched\n");
            return 0;
        } else {
            printf("Pattern Mismatched\n");
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
