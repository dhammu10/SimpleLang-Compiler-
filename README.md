# SimpleLang-Compiler-

This project is a **toy compiler** for a small C-like language called **SimpleLang**.  
It translates `.sl` source code into an **assembly-like language** for a hypothetical CPU.

---

## ✨ Features
- Variable declarations (`int x;`)
- Assignments (`x = 5 + y;`)
- Arithmetic expressions (`+`, `-`)
- Conditional `if` statements with equality (`if (a == b) { ... }`)
- Block statements (`{ ... }`)
- Assembly generation with instructions like:
  - `MOV`, `LOAD`, `STORE`
  - `ADD`, `SUB`
  - `CMP`, `JZ`, `JMP`, `HLT`

---

## 📂 Project Structure

    ├── compiler.cpp                                      # Compiler source code 
    ├── sample.sl                                         # Example SimpleLang program 
    └── output.asm                                        # Generated assembly code 

# Example
## sample.sl
      int x;
    int y;
    x = 10;
    y = x + 5;
    
    if (x == y) {
        y = y - 1;
    }

## output.asm
    MOV A, #10        ; load immediate
    STORE A, [10]   ; a = A
    MOV A, #20        ; load immediate
    STORE A, [11]   ; b = A
    LOAD A, [10]    ; load a
    ADD A, [11]     ; add b
    STORE A, [12]   ; c = A
    LOAD A, [12]    ; load c
    CMP A, #30          ; compare with 30
    JZ L0
    JMP L1
    L0:
        LOAD A, [12]    ; load c
        ADD A, #1        ; add immediate
        STORE A, [12]   ; c = A
    L1:
        HLT


## Simplified
    program      ::= { statement }
    statement    ::= declaration | assignment | if_statement | block
    declaration  ::= "int" IDENT ";"
    assignment   ::= IDENT "=" expr ";"
    if_statement ::= "if" "(" expr "==" expr ")" block
    block        ::= "{" { statement } "}"
    expr         ::= term { ("+" | "-") term }
    term         ::= IDENT | NUMBER
