#include "json.h"

#include <string>

using namespace std::literals;

namespace json {

    namespace {

        Node LoadNode(std::istream& input);

        Node LoadNull(std::istream& input) {
            const std::string null_str = "null"s;
            for (size_t i = 0; i < null_str.size(); i++) {
                if (null_str.at(i) == input.get()) {
                    continue;
                } else {
                    throw ParsingError("LoadNull parsing error"s);
                }
            }
            return {};
        }

        Node LoadArray(std::istream& input) {
            Array result;
            if (input.peek() == -1) {
                throw ParsingError("Array parsing error");
            }
            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            return Node(std::move(result));
        }

        // Считывает содержимое строкового литерала JSON-документа
        // Функцию следует использовать после считывания     открывающего символа ":
        std::string LoadString(std::istream& input) {

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    // Поток закончился до того, как встретили закрывающую кавычку?
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    // Встретили закрывающую кавычку
                    ++it;
                    break;
                } else if (ch == '\\') {
                    // Встретили начало escape-последовательности
                    ++it;
                    if (it == end) {
                        // Поток завершился сразу после символа обратной косой черты
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        // Встретили неизвестную escape-последовательность
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                } else if (ch == '\n' || ch == '\r') {
                    // Строковый литерал внутри- JSON не может прерываться символами \r или \n
                    throw ParsingError("Unexpected end of line"s);
                } else {
                    // Просто считываем очередной символ и помещаем его в результирующую строку
                    s.push_back(ch);
                }
                ++it;
            }
            return s;
        }

        Node LoadNumber(std::istream& input) {

            std::string parsed_num;

            // Считывает в parsed_num очередной символ из input
            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            // Считывает одну или более цифр в parsed_num из input
            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            // Парсим целую часть числа
            if (input.peek() == '0') {
                read_char();
                // После 0 в JSON не могут идти другие цифры
            } else {
                read_digits();
            }

            bool is_int = true;
            // Парсим дробную часть числа
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            // Парсим экспоненциальную часть числа
            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    // Сначала пробуем преобразовать строку в int
                    try {
                        return std::stoi(parsed_num);
                    } catch (...) {
                        // В случае неудачи, например, при переполнении,
                        // код ниже попробует преобразовать строку в double
                    }
                }
                return std::stod(parsed_num);
            } catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadBool(std::istream& input) {
            const std::string false_str = "false"s;
            const std::string true_str = "true"s;
            char c = input.get();
            bool value = (c == 't');
            std::string const* res = value ? &true_str : &false_str;
            for (size_t i = 1; i < res->size(); i++) {
                if (res->at(i) == input.get()) {
                    continue;
                } else {
                    throw ParsingError("Bool parsing error");
                }
            }
            return Node(value);
        }

        Node LoadDict(std::istream& input) {
            Dict result;
            if (input.peek() == -1) {
                throw ParsingError("Dictionary parsing error");
            }
            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                std::string key = LoadString(input);
                input >> c;
                result.insert({ std::move(key), LoadNode(input) });
            }
            return Node(std::move(result));
        }

        Node LoadNode(std::istream& input) {
            char c;
            input >> c;

            switch (c) {
            case 'n': {
                input.putback(c);
                return LoadNull(input);
            }
            case '"': {
                return LoadString(input);
            }
            case 't': {
            }
            case 'f': {
                input.putback(c);
                return LoadBool(input);
            }
            case '[': {
                return LoadArray(input);
            }
            case '{': {
                return LoadDict(input);
            }
            default:
            {
                input.putback(c);
                return LoadNumber(input);
            }
            }

        }

    }  // namespace

    Node::Node(std::nullptr_t null)
        : value_(null) {
    }

    Node::Node(Array array)
        : value_(std::move(array)) {
    }

    Node::Node(Dict map)
        : value_(std::move(map)) {
    }

    Node::Node(bool value)
        : value_(value) {
    }

    Node::Node(int value)
        : value_(value) {
    }

    Node::Node(double value)
        : value_(value) {
    }

    Node::Node(std::string value)
        : value_(std::move(value)) {
    }

    // ----- сообщают, хранится ли внутри значение некоторого типа -------
    bool Node::IsInt() const {
        return std::holds_alternative<int>(value_);
    }

    bool Node::IsDouble() const {
        return std::holds_alternative<int>(value_) || std::holds_alternative<double>(value_);
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(value_);
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(value_);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(value_);
    }

    bool Node::IsNull() const {
        return std::holds_alternative<std::nullptr_t>(value_);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(value_);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(value_);
    }

    // --------- возвращают хранящееся внутри Node значение заданного типа ------
    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Type is not int"s);
        }
        return std::get<int>(value_);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Type is not bool"s);
        }
        return std::get<bool>(value_);
    }

    double Node::AsDouble() const {
        if (!IsDouble()) {
            throw std::logic_error("Type is not int or double"s);
        }
        if (IsInt()) {
            return static_cast<double>(std::get<int>(value_));
        } else {
            return std::get<double>(value_);
        }
    }

    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Type is not string"s);
        }
        return std::get<std::string>(value_);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Type is not Array"s);
        }
        return std::get<Array>(value_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Type is not Dict"s);
        }
        return std::get<Dict>(value_);
    }
    // --------------------------------------------
    bool Node::operator==(const Node& other) const {
        return value_ == other.value_;
    }

    bool Node::operator!=(const Node& other) const {
        return !(value_ == other.value_);
    }

    const Node::Value& Node::GetValue() const { return value_; }

    Node::Value& Node::GetValue() { return value_; }

    Document::Document(Node root)
        : root_(std::move(root)) {
    }

    bool Document::operator==(const Document& other) const {
        return root_ == other.root_;
    }

    bool Document::operator!=(const Document& other) const {
        return !(root_ == other.root_);
    }


    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(std::istream& input) {
        return Document{ LoadNode(input) };
    }

    // -----------------------------------

    void PrintValue(std::nullptr_t, const PrintContext& ctx) {
        ctx.out << "null"sv;
    }

    void PrintValue(Array array, const PrintContext& ctx) {
        ctx.out << "[\n"sv;
        bool is_first = true;
        auto inner_ctx = ctx.Indented();
        for (const auto& elem : array) {
            if (is_first) {
                is_first = false;
            } else {
                ctx.out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintNode(elem, inner_ctx);
        }
        ctx.out << "\n"sv;
        ctx.PrintIndent();
        ctx.out << "]"sv;
    }

    void PrintValue(Dict map, const PrintContext& ctx) {
        ctx.out << "{\n"sv;
        bool is_first = true;
        auto inner_ctx = ctx.Indented();
        for (const auto& [key, value] : map) {
            if (is_first) {
                is_first = false;
            } else {
                ctx.out << ",\n"sv;
            }
            inner_ctx.PrintIndent();
            PrintValue(key, ctx);
            ctx.out << ": "sv;
            PrintNode(value, inner_ctx);
        }
        ctx.out << "\n"sv;
        ctx.PrintIndent();
        ctx.out << "}"sv;
    }

    void PrintValue(bool value, const PrintContext& ctx) {
        ctx.out << std::boolalpha << value;
    }

    void PrintValue(std::string value, const PrintContext& ctx) {
        ctx.out << "\""sv;
        for (const char& c : value) {
            if (c == '\n') {
                ctx.out << "\\n"sv;
                continue;
            }
            if (c == '\r') {
                ctx.out << "\\r"sv;
                continue;
            }
            if (c == '\"') {
                ctx.out << "\\"sv;
            }
            if (c == '\t') {
                ctx.out << "\t"sv;
                continue;
            }
            if (c == '\\') {
                ctx.out << "\\"sv;
            }
            ctx.out << c;
        }
        ctx.out << "\""sv;
    }

    void PrintNode(const Node& node, const PrintContext& ctx) {
        std::visit(
            [&ctx](const auto& value) {PrintValue(value, ctx); }, node.GetValue());
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintContext ctx{ output };
        PrintNode(doc.GetRoot(), ctx);
        ctx.out << "\n"sv;
    }

}  // namespace json