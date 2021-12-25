#include <fstream>
#include "lexer.cpp"

std::string ReadEntireFile(const char *path){
    std::fstream file(path);

    int size = 0;
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string buffer;
    buffer.resize(size);
    file.read(&buffer[0], size);
    return buffer;
}

int main() {
    auto src = ReadEntireFile("examples/test.e");
    Lexer lexer;
    lexer.DoLexicalAnalysis(src);
    return 0;
}
