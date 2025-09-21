#include <iostream> 
#include <fstream> 
#include <string> 
#include <vector> 
#include <cctype> 
#include <cstdarg> 
#include <cstdio> 
#include <cstdlib> 


enum class TokenType { 
INT, IDENT, NUMBER, 
ASSIGN, PLUS, MINUS, SEMI, 
LPAREN, RPAREN, LBRACE, RBRACE, 
IF, EQ, EOF_TOKEN, UNKNOWN
}; 


struct Token { 
TokenType type; 
std::string text; 
}; 

struct Var { 
std::string name; 
int addr; 
}; 


std::vector<Token> tokens; 
size_t tok_pos = 0; 
std::vector<Var> vars; 
int next_addr = 0x10;  
std::vector<std::string> asm_lines; 
int label_ctr = 0; 


void add_token(TokenType t, const std::string& s); 
void lexer(std::ifstream& f); 
int var_lookup(const std::string& name); 
int var_add(const std::string& name); 
void emit(const std::string& line); 
std::string newlabel(); 
Token* cur(); 
void eat(); 
bool accept(TokenType t); 
void expect(TokenType t); 
void parse_program(); 
void parse_statement(); 
void parse_block(); 
void parse_declaration(); 
void parse_assignment(); 
void parse_if(); 
void gen_load_operand_to_A_from_term(const Token& t); 
Token parse_term(); 
void parse_expr_and_emit(); 
 
void add_token(TokenType t, const std::string& s) { 
    tokens.push_back({t, s}); 
} 


void lexer(std::ifstream& f) { 
    char c; 
    while (f.get(c)) { 
        if (isspace(c)) continue; 

           if (isalpha(c) || c == '_') { 
            std::string buf; 
            buf += c; 

            while (f.get(c) && (isalnum(c) || c == '_')) { 
                buf += c; 
            } 

            if (f) f.unget(); 

            if (buf == "int") add_token(TokenType::INT, buf); 
            else if (buf == "if") add_token(TokenType::IF, buf); 
            else add_token(TokenType::IDENT, buf); 
            continue; 
        } 

         
        if (isdigit(c)) { 
            std::string buf; 
            buf += c; 

            while (f.get(c) && isdigit(c)) { 
                buf += c; 
            } 

            if (f) f.unget(); 
            add_token(TokenType::NUMBER, buf); 
            continue; 
        } 

         
        switch (c) { 
            case '=': { 
                char n = f.get(); 
                if (n == '=') { 
                    add_token(TokenType::EQ, "=="); 
                } else { 
                    if (f) f.unget(); 
                    add_token(TokenType::ASSIGN, "="); 
                } 
                break; 
            } 
            case '+': add_token(TokenType::PLUS, "+"); break; 
            case '-': add_token(TokenType::MINUS, "-"); break; 
            case ';': add_token(TokenType::SEMI, ";"); break; 
            case '(': add_token(TokenType::LPAREN, "("); break; 
            case ')': add_token(TokenType::RPAREN, ")"); break; 
            case '{': add_token(TokenType::LBRACE, "{"); break; 
            case '}': add_token(TokenType::RBRACE, "}"); break; 
            default: { 
                add_token(TokenType::UNKNOWN, std::string(1, c)); 
                break; 
            } 
        } 
    } 
    add_token(TokenType::EOF_TOKEN, ""); 
} 

 
int var_lookup(const std::string& name) { 
    for (const auto& var : vars) { 
        if (var.name == name) { 
            return var.addr; 
        } 
    } 
    return -1; 
} 

int var_add(const std::string& name) { 
    int v = var_lookup(name); 
    if (v >= 0) return v; 

    vars.push_back({name, next_addr++}); 
    return vars.back().addr; 
} 

 
void emit(const std::string& line) { 
asm_lines.push_back(line); 
} 
std::string newlabel() { 
return "L" + std::to_string(label_ctr++); 
} 
 
Token* cur() { 
if (tok_pos < tokens.size()) { 
return &tokens[tok_pos]; 
} 
return nullptr; 
} 
void eat() { 
if (tok_pos < tokens.size() && tokens[tok_pos].type != TokenType::EOF_TOKEN) { 
tok_pos++; 
} 
} 
bool accept(TokenType t) { 
if (cur() && cur()->type == t) { 
eat(); 
return true; 
} 
return false; 
} 
void expect(TokenType t) { 
    if (!accept(t)) { 
        std::cerr << "Parse error: expected token " << static_cast<int>(t)  
                  << " but got " << static_cast<int>(cur()->type)  
                  << " (" << cur()->text << ")\n"; 
        exit(1); 
    } 
} 


void gen_load_operand_to_A_from_term(const Token& t) { 
    if (t.type == TokenType::NUMBER) { 
        emit("    MOV A, #" + t.text + "        ; load immediate"); 
    } else if (t.type == TokenType::IDENT) { 
        int addr = var_lookup(t.text); 
        if (addr < 0) { 
            std::cerr << "Undefined variable " << t.text << "\n"; 
            exit(1); 
        } 
        char addr_str[10]; 
        snprintf(addr_str, sizeof(addr_str), "%02X", addr); 
        emit("    LOAD A, [" + std::string(addr_str) + "]    ; load " + t.text); 
    } else { 
        std::cerr << "Invalid term in expression\n"; 
        exit(1); 
    } 
} 

Token parse_term() { 
    Token t = *cur(); 
    if (t.type == TokenType::NUMBER || t.type == TokenType::IDENT) { 
        eat(); 
        return t; 
    } 
    std::cerr << "Parse error: expected term but got " << static_cast<int>(t.type)  
              << " (" << t.text << ")\n"; 
    exit(1); 
} 

void parse_expr_and_emit() { 
    Token first = parse_term(); 
    gen_load_operand_to_A_from_term(first); 

    while (cur() && (cur()->type == TokenType::PLUS || cur()->type == TokenType::MINUS)) { 
        TokenType op = cur()->type; 
        eat(); 
        Token t2 = parse_term(); 

        if (t2.type == TokenType::NUMBER) { 
            if (op == TokenType::PLUS) { 
                emit("    ADD A, #" + t2.text + "        ; add immediate"); 
            } else { 
                emit("    SUB A, #" + t2.text + "        ; sub immediate"); 
            } 
        } else { 
            int addr = var_lookup(t2.text); 
            if (addr < 0) { 
                std::cerr << "Undefined variable " << t2.text << "\n"; 
                exit(1); 
            } 

            char addr_str[10]; 
            snprintf(addr_str, sizeof(addr_str), "%02X", addr); 

            if (op == TokenType::PLUS) { 
                emit("    ADD A, [" + std::string(addr_str) + "]     ; add " + t2.text); 
            } else { 
                emit("    SUB A, [" + std::string(addr_str) + "]     ; sub " + t2.text); 
            } 
        } 
    } 
} 

 
void parse_declaration() { 
    expect(TokenType::INT); 
    if (!cur() || cur()->type != TokenType::IDENT) { 
        std::cerr << "Expected identifier in declaration\n"; 
        exit(1); 
    } 

    std::string name = cur()->text; 
    eat(); 

    var_add(name); 
    expect(TokenType::SEMI); 
} 

void parse_assignment() { 
    if (!cur() || cur()->type != TokenType::IDENT) { 
        std::cerr << "Expected identifier in assignment\n"; 
        exit(1); 
    } 

std::string varname = cur()->text; 
eat(); 
expect(TokenType::ASSIGN); 
parse_expr_and_emit(); 
int addr = var_lookup(varname); 
if (addr < 0) { 
std::cerr << "Undeclared variable in assignment: " << varname << "\n"; 
exit(1); 
} 
char addr_str[10]; 
snprintf(addr_str, sizeof(addr_str), "%02X", addr); 
emit("    STORE A, [" + std::string(addr_str) + "]   ; " + varname + " = A"); 
expect(TokenType::SEMI); 
} 
void parse_if() { 
expect(TokenType::IF); 
expect(TokenType::LPAREN); 
Token left = parse_term(); 
expect(TokenType::EQ); 
Token right = parse_term(); 
expect(TokenType::RPAREN); 
std::string lbl_else = newlabel(); 
std::string lbl_end = newlabel(); 
 
    gen_load_operand_to_A_from_term(left); 

 
    if (right.type == TokenType::NUMBER) { 
        emit("    CMP A, #" + right.text + "          ; compare with " + right.text); 
    } else { 
        int addr = var_lookup(right.text); 
        if (addr < 0) { 
            std::cerr << "Undefined variable " << right.text << "\n"; 
            exit(1); 
        } 
        char addr_str[10]; 
        snprintf(addr_str, sizeof(addr_str), "%02X", addr); 
        emit("    CMP A, [" + std::string(addr_str) + "]       ; compare with " + right.text); 
    } 


    emit("    JZ " + lbl_else); 
    emit("    JMP " + lbl_end); 


    emit(lbl_else + ":"); 
    parse_block(); 

 
    emit(lbl_end + ":"); 
} 

void parse_block() { 
    expect(TokenType::LBRACE); 
    while (cur() && cur()->type != TokenType::RBRACE && cur()->type != TokenType::EOF_TOKEN) { 
        parse_statement(); 
    } 
    expect(TokenType::RBRACE); 
} 

void parse_statement() { 
    if (accept(TokenType::INT)) { 
 
        tok_pos--; 
        parse_declaration(); 
    } else if (cur() && cur()->type == TokenType::IDENT) { 
        parse_assignment(); 
    } else if (accept(TokenType::IF)) { 
  
        tok_pos--; 
        parse_if(); 
    } else if (accept(TokenType::LBRACE)) { 
  
        tok_pos--; 
        parse_block(); 
    } else { 
        std::cerr << "Unknown statement start: " << static_cast<int>(cur()->type)  
                  << " (" << cur()->text << ")\n"; 
        exit(1); 
    } 
} 

void parse_program() { 
    while (cur() && cur()->type != TokenType::EOF_TOKEN) { 
        parse_statement(); 
    } 
} 

 
int main(int argc, char **argv) { 
    if (argc < 3) { 
        std::cerr << "Usage: " << argv[0] << " input.sl output.asm\n"; 
        return 1; 
    } 

    std::ifstream fin(argv[1]); 
    if (!fin) { 
        perror("Error opening input file"); 
        return 1; 
    } 

    lexer(fin); 
    fin.close(); 

 
    tok_pos = 0; 
    parse_program(); 

 
    emit("    HLT"); 


    std::ofstream fout(argv[2]); 
    if (!fout) { 
        perror("Error opening output file"); 
        return 1; 
    } 
for (const auto& line : asm_lines) { 
fout << line << "\n"; 
} 
fout.close(); 
std::cout << "Compilation successful. Generated " << asm_lines.size()  
<< " assembly lines in " << argv[2] << "\n"; 
return 0; 
} 
