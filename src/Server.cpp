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
    Literal, // match any literal character 
    StartAnchor, // force the match at the start only
    EndAnchor, // for the match at the end 
    PlusQuantifier, // one or more
    QuestionQuantifier // zero or one
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
        DBG_PRINT("Tokenize pattern: " << c);

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
        else if (c == '^'){
            toks.push_back({TokenType::StartAnchor, ""});
            DBG_PRINT("Token for Start Anchor created");
            i += 1;
        } 
        else if (c == '$'){
            DBG_PRINT("Token for End Anchor created");
            toks.push_back({TokenType::EndAnchor, ""});
            i += 1;
        }
        else if (c == '+'){
            DBG_PRINT("one or more quantifier created");
            toks.push_back({TokenType::PlusQuantifier, ""});
            i += 1;
        }
        else if (c == '?'){
            DBG_PRINT("zero or one quantifier created");
            toks.push_back({TokenType::QuestionQuantifier, ""});
            i += 1;
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

//atom mean the smallest unit
bool match_atom(const Token& tok, char ch){
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
            default: return false;        
        }
    return true;
}

// gready approach: consume max
// static function means internal linkage, only available within this cpp file
static size_t consume_max(const std::string& s, size_t i, const Token& atom){
    size_t k = 0, p = i;
    while (p < s.size() && match_atom(atom, s[p])){p++, k++;}
    return k;
}

// worker: i = input index, j = token index, start = original start (for ^)
static bool match_from(const std::string& s,
                       size_t i,
                       const std::vector<Token>& toks,
                       size_t j,
                       size_t start){    
    // std::size_t i = start;
    // std::size_t j = 0;
    // DBG_PRINT("s.size "<< s.size());


    while (j < toks.size()){
        DBG_PRINT("i = " << i << " j = " << j << " -> - v ");
        const Token& tok = toks[j];

        if (tok.type == TokenType::StartAnchor) {
            if (start != 0) return false;
            ++j;
            continue; // doesn’t consume a character
        }

        if (tok.type == TokenType::EndAnchor) {
            if (i != s.size()) return false;
            ++j;
            continue; // doesn’t consume a character
        }

        // PlusQuantifier takes effect on its preceding thing
        if (j + 1 < toks.size() && toks[j+1].type == TokenType::PlusQuantifier){
            if (!match_atom(tok, s[i])) return false;

            size_t max_k = consume_max(s, i, tok);
            
        // For loop to ensure corner cases: Pattern: a+ab Input: aaab, Work it out and you will see
            for (size_t k = max_k; k >= 1; k--){
                if (match_from(s, i+k, toks, j+2, start)) return true;
                if (k == 1) break;
            }
            return false;
        }
        // QuestionQuantifier , also try to be greedy
        if (j + 1 < toks.size() && toks[j+1].type == TokenType::QuestionQuantifier){
            // try taking 1 if possible
            if (i < s.size() && match_atom(tok, s[i])){
                if (match_from(s, i + 1, toks, j+2, start)) return true;
            }
            // try taking 0 (backtrack)
            if (match_from(s, i, toks, j+2, start)) return true;
            return false;
        }

        // Question
        if ( i >= s.size() || !match_atom(tok, s[i])) return false;
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
        if(match_from(input_line, pos, toks, 0, pos)){
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
