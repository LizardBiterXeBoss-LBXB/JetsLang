#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <cstdlib>
#include<cstdio>

// 全局变量存储
std::map<std::string, int> int_var;
std::map<std::string, std::string> str_var;
std::map<std::string, std::string> var_type;   // 记录每个变量的类型 "int" 或 "str"

bool error = false;   // 错误标志

// 去除字符串首尾空格
void trim(std::string &s) {
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    size_t end = s.find_last_not_of(" \t");
    s = s.substr(start, end - start + 1);
}

// 获取等号右侧的值（去除首尾空格，如果是字符串常量则去掉引号）
std::string parse_value(const std::string &s) {
    std::string val = s;
    trim(val);
    // 如果是字符串常量（以 " 开头和结尾）
    if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
        return val.substr(1, val.size() - 2);
    }
    return val;
}

// 检查变量是否存在，并返回其类型
bool var_exists(const std::string &name, std::string &type) {
    auto it = var_type.find(name);
    if (it != var_type.end()) {
        type = it->second;
        return true;
    }
    return false;
}

// 赋值操作：将 value 赋给变量 name，value 可以是数字、字符串常量或另一个变量名
void assign_var(const std::string &name, const std::string &value) {
    std::string type;
    if (!var_exists(name, type)) {
        std::cerr << "RUNTIME ERROR: 未声明的变量 " << name << "\n";
        error = true;
        return;
    }

    // 判断 value 是数字、字符串常量还是变量名
    std::string val_str = parse_value(value);
    if (type == "int") {
        // 如果 value 是另一个整数变量，则取其值
        if (var_type.find(val_str) != var_type.end() && var_type[val_str] == "int") {
            int_var[name] = int_var[val_str];
        } else {
            // 否则尝试转换为整数
            char *endptr;
            long num = std::strtol(val_str.c_str(), &endptr, 10);
            if (*endptr != '\0') {
                std::cerr << "RUNTIME ERROR: 无法将 '" << val_str << "' 赋给 int 变量 " << name << "\n";
                error = true;
            } else {
                int_var[name] = static_cast<int>(num);
            }
        }
    } else if (type == "str") {
        // 如果 value 是另一个字符串变量，则取其值
        if (var_type.find(val_str) != var_type.end() && var_type[val_str] == "str") {
            str_var[name] = str_var[val_str];
        } else {
            str_var[name] = val_str;   // 直接保存（已经去掉引号）
        }
    }
}

// 输出：支持输出字符串常量或变量
void print_statement(const std::string &arg) {
    std::string content = arg;
    trim(content);
    if (content.size() >= 2 && content.front() == '"' && content.back() == '"') {
        // 字符串常量
        std::cout << content.substr(1, content.size() - 2);
    } else {
        // 变量
        std::string type;
        if (!var_exists(content, type)) {
            std::cerr << "RUNTIME ERROR: 未声明的变量 " << content << "\n";
            error = true;
            return;
        }
        if (type == "int") {
            std::cout << int_var[content];
        } else if (type == "str") {
            std::cout << str_var[content];
        }
    }
    std::cout << std::endl;   // 每条 print 后换行
}

int main() {
    std::string ifn;
    std::cin >> ifn;
    std::ifstream inf(ifn);
    if (!inf.is_open()) {
        std::cerr << "无法打开文件 " << ifn << "\n";
        return 1;
    }

    std::string line;
    while (std::getline(inf, line)) {
        if (error) {
            std::exit(1);
        }

        trim(line);
        if (line.empty() || line[0] == '#')   // 忽略空行和注释（以 # 开头）
            continue;

        // ----- 处理变量声明 -----
        if (line.rfind("var ", 0) == 0) {
            std::string rest = line.substr(4);   // 去掉 "var "
            trim(rest);
            // 分离类型
            size_t space1 = rest.find(' ');
            if (space1 == std::string::npos) {
                std::cerr << "RUNTIME ERROR: var 语法错误\n";
                error = true;
                continue;
            }
            std::string type = rest.substr(0, space1);
            if (type != "int" && type != "str") {
                std::cerr << "RUNTIME ERROR: 未知类型 " << type << "\n";
                error = true;
                continue;
            }

            std::string rest2 = rest.substr(space1 + 1);
            trim(rest2);
            // 分离变量名和可能的初始值
            size_t space2 = rest2.find(' ');
            std::string name;
            std::string init_val;
            if (space2 == std::string::npos) {
                name = rest2;
                init_val = "";
            } else {
                name = rest2.substr(0, space2);
                std::string tail = rest2.substr(space2 + 1);
                trim(tail);
                // 检查是否有 '='
                if (tail.empty()) {
                    init_val = "";
                } else if (tail[0] == '=') {
                    init_val = tail.substr(1);
                    trim(init_val);
                } else {
                    std::cerr << "RUNTIME ERROR: 期望 '=' 或结束\n";
                    error = true;
                    continue;
                }
            }

            // 检查变量是否已声明
            if (var_type.find(name) != var_type.end()) {
                std::cerr << "RUNTIME ERROR: 变量 " << name << " 重复声明\n";
                error = true;
                continue;
            }

            // 记录类型并赋默认值
            var_type[name] = type;
            if (type == "int") {
                int_var[name] = 0;
                if (!init_val.empty()) {
                    assign_var(name, init_val);   // 但 assign_var 会检查类型，可能会出错，但此时变量已存在
                    if (error) continue;
                }
            } else { // str
                str_var[name] = "";
                if (!init_val.empty()) {
                    assign_var(name, init_val);
                    if (error) continue;
                }
            }
        }
        // ----- 处理赋值语句 -----
        else if (line.find('=') != std::string::npos) {
            size_t eq = line.find('=');
            std::string left = line.substr(0, eq);
            trim(left);
            std::string right = line.substr(eq + 1);
            trim(right);
            if (left.empty() || right.empty()) {
                std::cerr << "RUNTIME ERROR: 赋值语法错误\n";
                error = true;
                continue;
            }
            // 检查左侧变量是否已声明
            std::string type;
            if (!var_exists(left, type)) {
                std::cerr << "RUNTIME ERROR: 未声明的变量 " << left << "\n";
                error = true;
                continue;
            }
            assign_var(left, right);
        }
        // ----- 处理输出语句 -----
        else if (line.rfind("print ", 0) == 0) {
            std::string arg = line.substr(6);   // 去掉 "print "
            print_statement(arg);
        }
        // ----- 未知语句 -----
        else {
            std::cerr << "RUNTIME ERROR: 未知语句 '" << line << "'\n";
            error = true;
        }
    }

    if (error) std::exit(1);
    getchar();
    return 0;
}
