/*
 * Copyright (C) Rida Bazzi
 *
 * Do not share this file with anyone
 */
#include <iostream>
#include <istream>
#include <vector>
#include <string>
#include <cctype>

#include "lexer.h"
#include "inputbuf.h"

using namespace std;

string reserved[] = {
    "END_OF_FILE",
    "IF", "WHILE", "DO", "THEN", "PRINT",
    "PLUS", "MINUS", "DIV", "MULT",
    "EQUAL", "COLON", "COMMA", "SEMICOLON",
    "LBRAC", "RBRAC", "LPAREN", "RPAREN",
    "NOTEQUAL", "GREATER", "LESS", "LTEQ", "GTEQ",
    "DOT", "NUM", "ID", "ERROR", "REALNUM",
    "BASE08NUM", "BASE16NUM" // TODO: Add labels for new token types here (as string)
};

#define KEYWORDS_COUNT 5
string keyword[] = {"IF", "WHILE", "DO", "THEN", "PRINT"};

void Token::Print()
{
    cout << "{" << this->lexeme << " , "
         << reserved[(int)this->token_type] << " , "
         << this->line_no << "}\n";
}

LexicalAnalyzer::LexicalAnalyzer()
{
    this->line_no = 1;
    tmp.lexeme = "";
    tmp.line_no = 1;
    tmp.token_type = ERROR;
}

bool LexicalAnalyzer::SkipSpace()
{
    char c;
    bool space_encountered = false;

    input.GetChar(c);
    line_no += (c == '\n');

    while (!input.EndOfInput() && isspace(c))
    {
        space_encountered = true;
        input.GetChar(c);
        line_no += (c == '\n');
    }

    if (!input.EndOfInput())
    {
        input.UngetChar(c);
    }
    return space_encountered;
}

bool LexicalAnalyzer::IsKeyword(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++)
    {
        if (s == keyword[i])
        {
            return true;
        }
    }
    return false;
}

TokenType LexicalAnalyzer::FindKeywordIndex(string s)
{
    for (int i = 0; i < KEYWORDS_COUNT; i++)
    {
        if (s == keyword[i])
        {
            return (TokenType)(i + 1);
        }
    }
    return ERROR;
}

/**
 * Is called when the next character in the input buffer is determined to be a digit. This func will determine if it is a num, base 16 num, base 8 num, real num, or is an invalid token (bc those are the only allowed tokens that start with a digit)
 */
Token LexicalAnalyzer::ScanNumber()
{
    char c;

    input.GetChar(c);
    if (isdigit(c))
    {
        if (c == '0')
        {
            tmp.lexeme = "0";
        }
        else
        {
            tmp.lexeme = "";

            //general loop which will end when a non-digit is encountered. A non-digit could mean various things, such as a hex digit >= 10, a base suffix for base 8 and base 16 nums, a period indicating a real number, or just a regular letter or symbol following a number. Each of these cases is checked below 
            while (!input.EndOfInput() && isdigit(c))
            {
                tmp.lexeme += c;
                input.GetChar(c);
            }

            //this section determines if this is a valid hex num or not
            if (c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F')
            {
                string test = "";
                while (!input.EndOfInput() && (isdigit(c) || (c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F')))
                {
                    test += c;
                    input.GetChar(c);
                }
                if (c == 'x')
                {
                    string base = "";
                    base += c;
                    input.GetChar(c);
                    base += c;
                    input.GetChar(c);
                    base += c;

                    if (base == "x16") // we know its a hex number, so make the lexeme complete and hand it off
                    {
                        tmp.lexeme += test;
                        input.UngetString(base);
                    }
                    else
                    {
                        input.UngetString(base);
                        input.UngetString(test);
                    }
                }
                else
                {
                    input.UngetChar(c);
                    input.UngetString(test);
                }
            }
            else if (!input.EndOfInput())
            {
                input.UngetChar(c);
            }
        }

        input.GetChar(c);

        //in this case the main loop stopped because we have encountered a base suffix OR just a regular letter x
        if (c == 'x')
        {
            bool validbase = false; // assume the chars after x aren't a valid base
            tmp.lexeme += c;
            string base = "";
            input.GetChar(c);
            base += c;
            input.GetChar(c);
            base += c;
            if (base == "08")
            {
                validbase = true;
                for (int i = 0; i < tmp.lexeme.size() - 1; i++)
                {
                    if (tmp.lexeme.at(i) - '0' > 7)
                    {
                        validbase = false;
                        break;
                    }
                }
                if (validbase == true)
                {
                    tmp.lexeme += base;
                    tmp.token_type = BASE08NUM;
                }
            }
            else if (base == "16") // case of base 16 number where there are no occurences of A-F
            {
                validbase = true;
                tmp.lexeme += base;
                tmp.token_type = BASE16NUM;
            }

            //x is just a regular letter in this case, not a base suffix, so the resulting token is just a NUM
            if (validbase == false)
            {
                input.UngetString(base);
                input.UngetChar(tmp.lexeme.back());
                tmp.lexeme.pop_back();
                tmp.token_type = NUM;
                // what to classify it as if arbitrary base is put after?
            }
        }
        
        else if (c == '.')
        { 
            tmp.lexeme += c;
            input.GetChar(c);
            if (isdigit(c))
            {
                while (!input.EndOfInput() && isdigit(c))
                {
                    tmp.lexeme += c;
                    input.GetChar(c);
                }
                if (!input.EndOfInput())
                {
                    input.UngetChar(c);
                }
                tmp.token_type = REALNUM;
            }
            else
            {
                input.UngetChar(c);
                input.UngetChar(tmp.lexeme.back());
                tmp.lexeme.pop_back();
                tmp.token_type = NUM;
            }
        }
        else
        {
            input.UngetChar(c);
            tmp.token_type = NUM;
        }
        tmp.line_no = line_no;
        return tmp;
    }
    else
    {
        if (!input.EndOfInput())
        {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
        tmp.line_no = line_no;
        return tmp;
    }
}

/**
 * Is called when the next character in the input buffer is determined to be a letter. It will determine if this character is part of a token that is either a hex number or a string (type BASE16NUM or type ID)
 */
Token LexicalAnalyzer::ScanIdOrKeyword()
{
    char c;
    input.GetChar(c);
    bool validbase = false;

    if (isalpha(c))
    {
        tmp.lexeme = "";

        //this token starts with a letter, which, according to the grammar given to us, could be a hex num, so this section checks if the token is a hex num
        while (!input.EndOfInput() && (isdigit(c) || c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F'))
        {
            tmp.lexeme += c;
            input.GetChar(c);
        }
        if (c == 'x')
        {
            string base = "";
            int counter = 0;
            while (!input.EndOfInput() && counter < 3){
                base += c;
                input.GetChar(c);
                counter++;
                //std::cout << counter << endl;
            }        
            //input.GetChar(c); //this is to check if there's a space after x16- apparrntly only counts as a hex # if there's a space after
            if (base == "x16" && (c == ' ' || input.EndOfInput() || c == '\n')) /*technically needs to check for more than these (like \t), but whatev*/
            {
                validbase = true;
                tmp.lexeme += base;
                tmp.token_type = BASE16NUM;
                input.UngetChar(c); //unget the space we checked 
            }
            else
            {
                
                input.UngetChar(c); //unget the last char/EOF 
                input.UngetString(base);
                input.GetChar(c); //only want to get another char if our base didnt work, so have to do it here as opposed to outside
            }
        }
        //at this point, we know its not a hex num, and, according to the grammar, the only other possible token starting w/ a letter is a regular string (has the type ID), so we just get the rest of the string, determine if its a reserved keyword, and init the token
        if (!validbase)
        {
            while (!input.EndOfInput() && isalnum(c))
            {
                tmp.lexeme += c;
                input.GetChar(c);
            }
            if (!input.EndOfInput())
            {
                input.UngetChar(c);
            }
            tmp.token_type = ID;
        }
        tmp.line_no = line_no;

        //determines if this a reserved string in the grammar
        if (IsKeyword(tmp.lexeme))
            tmp.token_type = FindKeywordIndex(tmp.lexeme);
    }
    else
    {
        if (!input.EndOfInput())
        {
            input.UngetChar(c);
        }
        tmp.lexeme = "";
        tmp.token_type = ERROR;
    }
    return tmp;
}

// you should unget tokens in the reverse order in which they
// are obtained. If you execute
//
//    t1 = lexer.GetToken();
//    t2 = lexer.GetToken();
//    t3 = lexer.GetToken();
//
// in this order, you should execute
//
//    lexer.UngetToken(t3);
//    lexer.UngetToken(t2);
//    lexer.UngetToken(t1);
//
// if you want to unget all three tokens. Note that it does not
// make sense to unget t1 without first ungetting t2 and t3
//
TokenType LexicalAnalyzer::UngetToken(Token tok)
{
    tokens.push_back(tok);
    ;
    return tok.token_type;
}

/**
 * Determines and initializes the next token in the input sequence 
 */
Token LexicalAnalyzer::GetToken()
{
    char c;

    // if there are tokens that were previously
    // stored due to UngetToken(), pop a token and
    // return it without reading from input
    if (!tokens.empty())
    {
        tmp = tokens.back();
        tokens.pop_back();
        return tmp;
    }

    //else, get the first non-space characater from stdin 
    SkipSpace();
    tmp.lexeme = "";
    tmp.line_no = line_no;
    input.GetChar(c);

    //and determine its type (and value, if necessary) by comparing the fetched stdin char to a pre-defined set of possible accepted chars, and initialize the Token this func will return with these values
    switch (c)
    {
    case '.':
        tmp.token_type = DOT;
        return tmp;
    case '+':
        tmp.token_type = PLUS;
        return tmp;
    case '-':
        tmp.token_type = MINUS;
        return tmp;
    case '/':
        tmp.token_type = DIV;
        return tmp;
    case '*':
        tmp.token_type = MULT;
        return tmp;
    case '=':
        tmp.token_type = EQUAL;
        return tmp;
    case ':':
        tmp.token_type = COLON;
        return tmp;
    case ',':
        tmp.token_type = COMMA;
        return tmp;
    case ';':
        tmp.token_type = SEMICOLON;
        return tmp;
    case '[':
        tmp.token_type = LBRAC;
        return tmp;
    case ']':
        tmp.token_type = RBRAC;
        return tmp;
    case '(':
        tmp.token_type = LPAREN;
        return tmp;
    case ')':
        tmp.token_type = RPAREN;
        return tmp;
    case '<':
        input.GetChar(c);
        if (c == '=')
        {
            tmp.token_type = LTEQ;
        }
        else if (c == '>')
        {
            tmp.token_type = NOTEQUAL;
        }
        else
        {
            if (!input.EndOfInput())
            {
                input.UngetChar(c);
            }
            tmp.token_type = LESS;
        }
        return tmp;
    case '>':
        input.GetChar(c);
        if (c == '=')
        {
            tmp.token_type = GTEQ;
        }
        else
        {
            if (!input.EndOfInput())
            {
                input.UngetChar(c);
            }
            tmp.token_type = GREATER;
        }
        return tmp;
    default:
        if (isdigit(c))
        {
            input.UngetChar(c);
            return ScanNumber();
        }
        else if (isalpha(c))
        {
            input.UngetChar(c);
            return ScanIdOrKeyword();
        }
        else if (input.EndOfInput())
            tmp.token_type = END_OF_FILE;
        else
            tmp.token_type = ERROR;

        return tmp;
    }
}

int main()
{
    /**
     * lexer has an input buffer which holds, or fetches if none are buffered, the next characters in the input buffer. A series of checks are performed on the next char in the input buffer to determine its token type, and when found has all the chars belonging to this token read from the input buff and a Token created to represent it. This application simply creates the Tokens from raw input according to a pre-defined grammar and prints them
     */
    LexicalAnalyzer lexer;
    Token token;

    token = lexer.GetToken();
    token.Print();
    while (token.token_type != END_OF_FILE)
    {
        token = lexer.GetToken();
        token.Print();
    }
}
