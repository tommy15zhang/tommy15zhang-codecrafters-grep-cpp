#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <fstream>
#include <optional>
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
    QuestionQuantifier, // zero or one
    AnyChar,
    LeftParen, // (
    RigthParen, // )
    Alternation, // |
    BackRef // \1 backreference: reuse a captured group 
};

struct Token
{
    TokenType type;
    std::string data; //for CharClass, NegCharClass and Literal
};


 // only for the pattern
 //const std::string& -> I don’t want to copy the string, but I promise not to change it.
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
            } else if (next >= '1' && next <= '9'){
                size_t j = i + 1; // start at first digit
                while (j < n && std::isdigit(static_cast<unsigned char>(pattern[j]))) {
                    ++j;
                }
                // digits are [i+1, j)
                toks.push_back({TokenType::BackRef, std::string(pattern.begin() + (i + 1),
                                                                pattern.begin() + j)});
                i = j;            // consume '\' and all digits
                DBG_PRINT("Created BackRef token: \\" << toks.back().data);
                continue;
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
        else if (c == '.'){
            DBG_PRINT("matches any character token created");
            toks.push_back({TokenType::AnyChar, ""});
            i += 1;
        }
        else if (c == '('){
            toks.push_back({TokenType::LeftParen, ""});
            i += 1;
        }
        else if (c == ')'){
            toks.push_back({TokenType::RigthParen, ""});
            i += 1;
        }
        else if (c == '|'){
            toks.push_back({TokenType::Alternation, ""});
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
            case TokenType::AnyChar:
                DBG_PRINT("Checking any char against char: " << ch);
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

static std::vector<std::pair<size_t, size_t>>
split_alts(const std::vector<Token>& toks, size_t L, size_t R){
    std::vector<std::pair<size_t, size_t>> parts;
    size_t depth = 0, start = L;
    for (size_t k = L; k < R; ++k){
        if (toks[k].type == TokenType::LeftParen) depth++;
        else if (toks[k].type == TokenType::RigthParen) depth--;
        else if (toks[k].type == TokenType::Alternation && depth==0){
            parts.push_back({start, k});
            start = k + 1;
        } 
    }
    parts.push_back({start, R});
    return parts;
}
static size_t find_rparen(const std::vector<Token>& toks, size_t open_j){
    size_t depth = 0;
    for (size_t k = open_j; k < toks.size(); ++k){
        if (toks[k].type == TokenType::LeftParen) depth++;
        else if (toks[k].type == TokenType::RigthParen){
            if (--depth == 0) return k; 
        }
    }
    throw std::runtime_error("Unmatched '('");
}


static std::vector<int> number_groups(const std::vector<Token>& toks, int& max_gid){
    std::vector<int> gid_at_open(toks.size(), -1);
    int next = 1;
    std::vector<size_t> stack;
    for (size_t j = 0; j < toks.size(); ++j){
        if (toks[j].type == TokenType::LeftParen){
            gid_at_open[j] = next++;
            stack.push_back(j);
        } else if (toks[j].type == TokenType::RigthParen){
            if (stack.empty()) throw std::runtime_error("Unmatched ')");
            stack.pop_back();
        }
    }
    if (!stack.empty()) throw std::runtime_error("Unmatched ')");
    max_gid = next - 1; //for debug purpose
    return gid_at_open;
}
struct MatchCtx {
    std::vector<std::optional<std::string>> groups;
};

struct SliceResult {
    bool ok; 
    size_t next_i;
    MatchCtx ctx;
};

static SliceResult match_slice (const std::string& s, size_t i, const std::vector<Token>& toks, size_t j, size_t end_j, size_t start, MatchCtx ctx, std::vector<int>& gid_at_open, int max_gid){
    // Try to match the sub-pattern represented by tokens toks[j ... end_j] against the input string s starting at character index i
    // Return a struct {contain 2 values}. Ok: did this slice of pattern match successfully. next_i: if matched, where in th einput the match ended
    size_t depth = 0;
    bool has_bar = false;
    for (size_t k = j; k < end_j; ++k) {
        if (toks[k].type == TokenType::LeftParen){
            ++depth;
            // DBG_PRINT("Found (");
            // DBG_PRINT("Depth: " << depth);
        }
        else if (toks[k].type == TokenType::RigthParen){
            --depth;
            // DBG_PRINT("Found )");
            // DBG_PRINT("Depth: " << depth);
        }
        else if (toks[k].type == TokenType::Alternation && depth == 0) {
            // DBG_PRINT("Found |");
            // DBG_PRINT("Depth " << depth);
            has_bar = true; break;
        }
    }
    if (has_bar) {
        auto parts = split_alts(toks, j, end_j); // split on top-level '|'
        for (auto [a, b] : parts) {
            auto sub = match_slice(s, i, toks, a, b, start, ctx, gid_at_open, max_gid);
            if (sub.ok) return sub;   // succeed on the first branch that works
        }
        return {false, i, ctx};            // only fail after trying all branch
    }
    while (j < end_j){
        const Token& tok = toks[j];
        // anchors work the same
        if (tok.type == TokenType::StartAnchor){ if (start != 0) return {false, i, ctx}; ++j; continue; }
        if (tok.type == TokenType::EndAnchor){ if (i != s.size()) return {false, i, ctx}; ++j; continue; }

        // quantifiers on atoms inside a slice (your existing + / ? branches)
        if (j + 1 < end_j && toks[j+1].type == TokenType::PlusQuantifier){
            if (i >= s.size() || !match_atom(tok, s[i])) return {false, i, ctx};
            size_t max_k = consume_max(s, i, tok);
            for (size_t k = max_k; k >= 1; --k){
                auto sub = match_slice(s, i + k, toks, j + 2, end_j, start, ctx, gid_at_open, max_gid);
                if (sub.ok) return sub;
                if (k == 1) break;
            }
            return {false,i, ctx};
        }
        if (j + 1 < end_j && toks[j+1].type == TokenType::QuestionQuantifier){
            if (i < s.size() && match_atom(tok, s[i])){
                auto sub1 = match_slice(s, i + 1, toks, j + 2, end_j, start, ctx, gid_at_open, max_gid);
                if (sub1.ok) return sub1;
            }
            auto sub0 = match_slice(s, i, toks, j + 2, end_j, start, ctx, gid_at_open, max_gid);
            if (sub0.ok) return sub0;
            return {false,i, ctx};
        }
        if (tok.type == TokenType::BackRef){
            int gid = std::stoi(tok.data);
            DBG_PRINT("Matching BackRef \\" << gid 
                    << " ; has=" << (gid < (int)ctx.groups.size() && ctx.groups[gid].has_value()));
            if (gid <= 0 || gid >= (int)ctx.groups.size() || !ctx.groups[gid].has_value())
                return {false, i, ctx};
            const std::string& pat = *ctx.groups[gid];
            DBG_PRINT("BackRef text = \"" << pat << "\" at input pos " << i);
            if (i + pat.size() > s.size()) return {false, i, ctx};
            if (s.compare(i, pat.size(), pat) != 0) return {false, i, ctx};
            i += pat.size(); ++j; continue;
        }
        if (tok.type == TokenType::LeftParen){
            size_t r = find_rparen(toks, j);
            int gid = gid_at_open[j];

            // Match the group interior [j+1, r) exactly once from input index `pos`.
            auto run_group_once = [&](size_t pos, MatchCtx& ctx_in, size_t& out_next_i, MatchCtx& out_ctx) -> bool {
                auto sub = match_slice(s, pos, toks, j + 1, r, start, ctx_in, gid_at_open, max_gid);
                if (!sub.ok) return false;
                out_next_i = sub.next_i;  // where the group finished in the input
                // next_i tell exactly where to continue after the group
                out_ctx = sub.ctx;

                // if we haven't set g1 yest, treat this pair as group 1 for this stage:
                if (gid > 0) out_ctx.groups[gid] = s.substr(pos, out_next_i - pos);
                return true;
            };

            // IMPORTANT: look for quantifier relative to this slice's end (end_j), not toks.size().
            bool has_plus = (r + 1 < end_j && toks[r + 1].type == TokenType::PlusQuantifier);
            bool has_q    = (r + 1 < end_j && toks[r + 1].type == TokenType::QuestionQuantifier);

            if (has_plus) {
                // Greedy repeat the whole group and record every end position.
                std::vector<std::pair<size_t, MatchCtx>> ends;
                size_t cur = i, next = i;
                MatchCtx cur_ctx = ctx;
                while (run_group_once(cur, cur_ctx, next, cur_ctx)) {
                    if (next == cur) break;        // safety against empty group
                    ends.push_back({next, cur_ctx});
                    cur = next;
                }
                if (ends.empty()) return {false, i, ctx}; // '+' requires at least one

                // Backtrack: try k repetitions from max down to 1.
                for (size_t k = ends.size(); k >= 1; --k) {
                    size_t after = ends[k - 1].first;
                    MatchCtx after_ctx = ends[k-1].second;
                    auto cont = match_slice(s, after, toks, r + 2, end_j, start, after_ctx, gid_at_open, max_gid); // skip ')' and '+'
                    if (cont.ok) return cont;
                    if (k == 1) break; // prevent size_t underflow
                }
                return {false, i, ctx};
            }
            else if (has_q) {
                // Try once (greedy)…
                size_t after_once;
                MatchCtx ctx_after_once;
                if (run_group_once(i, ctx, after_once, ctx_after_once)) {
                    auto cont1 = match_slice(s, after_once, toks, r + 2, end_j, start, ctx_after_once, gid_at_open, max_gid);
                    if (cont1.ok) return cont1;
                }
                // …or skip it.
                return match_slice(s, i, toks, r + 2, end_j, start, ctx, gid_at_open, max_gid);
            }
            else {
                // No quantifier: match exactly once and continue within this slice.
                size_t after_once;
                MatchCtx ctx_after;
                if (!run_group_once(i, ctx, after_once, ctx_after)) return {false, i, ctx};
                i = after_once;
                ctx = ctx_after;
                j = r + 1;  // move past ')'
                continue;   // keep matching the rest of the slice
            }
        }
        // regular atom
        if (i >= s.size() || !match_atom(tok, s[i])) return {false, i, ctx};
        ++i; ++j;
    }
    return {true, i, ctx};
}

static bool match_from(const std::string& s,
                       size_t i,
                       const std::vector<Token>& toks,
                       size_t j,
                       size_t start,
                       std::vector<int> gid_at_open,
                        int max_gid){
    MatchCtx ctx; ctx.groups.resize(max_gid + 1);            
    auto sub = match_slice(s, i, toks, 0, toks.size(), start, ctx, gid_at_open, max_gid);
    return sub.ok;
}
bool match_pattern(const std::string& input_line, const std::string& pattern) {
    // Tokenize once
    auto toks = tokenize(pattern);
    int max_gid = 0;
    auto gid_at_open = number_groups(toks, max_gid);
    DBG_PRINT("Max_gid: " << max_gid);
    if(toks.empty()) return true; //empty pattern matches trivalliy

    for (std::size_t pos = 0; pos < input_line.size(); pos++){
        if(match_from(input_line, pos, toks, 0, pos, gid_at_open, max_gid)){
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

    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: " << argv[0] << " -E <Pattern> [file]" << std::endl;
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }


    std::string flag = argv[1];
    std::string pattern = argv[2];
    

    if (flag != "-E"){
        std::cerr << "Expected frist argument to be '-E'" << std::endl;
        return 1;
    }

    
    std::string input_line;
    if (argc == 4) {
        const char* filename = argv[3];
        std::ifstream in(filename);
        if (!in) {
            std::cerr << "Error: Cannot Open File: " << filename << std::endl;
            return 1;
        }
        std::getline(in, input_line);
    } else {
        std::getline(std::cin, input_line);
    }

    DBG_PRINT("Tokenizing pattern: " << pattern);

    try {
        if (match_pattern(input_line, pattern)) {
            std::cout << input_line << '\n';
            return 0;
        } else {
            return 1;
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
