#include <string>
#include <vector>
#include <array>
#include <optional>
#include <utility>
#include <assert.h>
#include <memory>
#include <iostream>

bool IsDigit(char ch){
    if(ch >= '0' && ch <= '9')
        return true;
    return false;
}

bool IsASCIILetter(char ch){
    return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_';
}

class CharacterStream{
private:
    const char *Data;
    size_t Size;
    size_t Position;
public:
    CharacterStream(const char *data, size_t size):
        Data(data),
        Size(size),
        Position(0)
    {}

    CharacterStream(const CharacterStream &) = default;

    CharacterStream &operator=(const CharacterStream &) = default;

    bool operator==(const CharacterStream &other){
        assert(Data == other.Data && Size == other.Size);
        return Position == other.Position;
    }

    bool operator!=(const CharacterStream &other){
        return !(*this == other);
    }

    bool IsEmpty()const{
        return Size == Position;
    }

    char ConsumeChar(){
        if(IsEmpty())
            return 0;
        return Data[Position++];
    }

    char PeekChar(){
        if(IsEmpty())
            return 0;
        return Data[Position];
    }
};

template<size_t CapacityValue>
class InputBuffer{
private:
    char m_Data[CapacityValue]{};
    size_t m_Size = 0;
public:
    InputBuffer(){
        m_Data[0] = 0;
    }

    void Add(char ch){
        m_Data[m_Size++] = ch;
    }

    const char *Data()const{
        return m_Data;
    }

    size_t Length()const{
        return m_Size;
    }
};

enum LexemeType{
    Identifier,
    Colon,
    Semicolon,
    Plus,
    Minus,
    Divide,
    Multiply,
    Equal,
    Int,
    IntLiteral,
};

struct Lexeme{
    const LexemeType Type;
    const size_t Data;

    Lexeme(LexemeType type, size_t data = -1):
        Type(type),
        Data(data)
    {}
};

class LexemeReader{
public:

    virtual std::optional<std::pair<Lexeme, CharacterStream>> TryRead(CharacterStream stream) = 0;
};

class IdentifierTable{
public:
    static constexpr size_t MaxIdentifierSize = 256;
private:
    struct Identifier{
        size_t Begin = 0;
        size_t Size  = 0;
    };
    std::vector<char> m_Data;
    std::vector<Identifier> m_Identifiers;
public:
    IdentifierTable(){
        m_Data.reserve(200000);
    }
    size_t Add(const char *identifier, size_t size){
        size_t begin = m_Data.size();

        for(int i = 0; i<size; i++)
            m_Data.push_back(identifier[i]);
        m_Data.push_back(0);

        m_Identifiers.push_back({begin, size});

        return m_Identifiers.size() - 1;//last index
    }

    std::vector<std::string_view> Identifiers()const{
        std::vector<std::string_view> views;
        views.reserve(m_Identifiers.size());

        for(auto id: m_Identifiers)
            views.push_back({&m_Data[id.Begin], id.Size});

        return views;
    }
};

class IdentifierLexemeReader: public LexemeReader{
private:
    IdentifierTable &m_Table;
public:
    IdentifierLexemeReader(IdentifierTable &table):
        m_Table(table)
    {}

    std::optional<std::pair<Lexeme, CharacterStream>> TryRead(CharacterStream stream)override{
        if(!IsASCIILetter(stream.PeekChar()))
            return {};
        InputBuffer<IdentifierTable::MaxIdentifierSize> buffer;

        for(;;){
            char ch = stream.PeekChar();
            if(!IsASCIILetter(ch) && !IsDigit(ch))
                break;

            buffer.Add(stream.ConsumeChar());
        }

        auto index = m_Table.Add(buffer.Data(), buffer.Length());

        return {{Lexeme(LexemeType::Identifier, index), stream}};
    }
};

class SingleCharacterLexemeReader: public LexemeReader{
    static constexpr std::pair<char, LexemeType> s_SingleCharacterLexemes[] = {
        {':', LexemeType::Colon},
        {';', LexemeType::Semicolon},
        {'=', LexemeType::Equal},
        {'+', LexemeType::Plus},
        {'-', LexemeType::Minus},
        {'*', LexemeType::Divide},
        {'/', LexemeType::Multiply},
    };
public:
    std::optional<std::pair<Lexeme, CharacterStream>> TryRead(CharacterStream stream)override {
        char ch = stream.ConsumeChar();

        for(auto [character, type]: s_SingleCharacterLexemes)
            if(ch == character)
                return {{Lexeme(type), stream}};
        return {};
    }
};

class IntegerLiteralLexemeReader: public LexemeReader{
public:
    std::optional<std::pair<Lexeme, CharacterStream>> TryRead(CharacterStream stream)override {
        while(IsDigit(stream.PeekChar()))
            stream.ConsumeChar();
        return {{Lexeme(LexemeType::IntLiteral), stream}};
    }
};

class Lexer{
private:
    IdentifierTable m_IdentifierTable;

    std::vector<std::unique_ptr<LexemeReader>> m_Readers;

    std::vector<Lexeme> m_Lexemes;
public:
    Lexer(){
        m_Readers.push_back(std::make_unique<SingleCharacterLexemeReader>());
        m_Readers.push_back(std::make_unique<IdentifierLexemeReader>(m_IdentifierTable));
        m_Readers.push_back(std::make_unique<IntegerLiteralLexemeReader>());
    }

    void DoLexicalAnalysis(const std::string &sources){
        CharacterStream stream(sources.data(), sources.size());

        while(!stream.IsEmpty()) {
            bool lexed = false;
            for (auto &reader: m_Readers) {
                auto result = reader->TryRead(stream);

                if (result.has_value()) {
                    stream = result->second;
                    m_Lexemes.push_back(result->first);
                    lexed = true;
                    break;
                }
                std::cout << "Tick" << std::endl;
            }
            assert(lexed);
            while(!stream.IsEmpty() && (stream.PeekChar() == '\n'
            || stream.PeekChar() == '\t' || stream.PeekChar() == ' '
            || stream.PeekChar() == '\r' || stream.PeekChar() == '\0'))
                stream.ConsumeChar();
        }

        for(std::string_view id: m_IdentifierTable.Identifiers()){
            for(auto ch: id)
                std::cout << ch;
            std::cout << '\n';
        }

        static constexpr const char *s_LexemeNames[]={
            "Identifier",
            "Colon",
            "Semicolon",
            "Plus",
            "Minus",
            "Divide",
            "Multiply",
            "Equal",
            "Int",
            "IntLiteral"
        };
        for(auto l: m_Lexemes)
            std::cout << s_LexemeNames[l.Type] << '\n';
    }
};
