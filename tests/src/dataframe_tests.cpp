#include <Lunaris/socket.h>

#include <random>
#include <iostream>

using dframe = Lunaris::Socket::DataFrame;

template<typename T>
T gen_rand_print() {
    T val = static_cast<T>(rand()) * static_cast<T>(rand());
    std::cout << "__ rand: gen -> " << val << "\n";
    return val;
}

void test1();

int main()
{
    try {
        test1();
    }
    catch(const std::exception& e) {
        fprintf(stderr, "Failed on test: %s\n", e.what());
        return 1;
    }

    return 0;
}

void test1()
{
    srand(126); // deterministic test, repeatable
    dframe frame;
    constexpr const char base_str[] = "This is a string `with some?! \ncharacters \t\raround\0 This should be there too!";

    const auto exp_0 = gen_rand_print<uint16_t>();
    const auto exp_1 = gen_rand_print<uint32_t>();
    const auto exp_2 = gen_rand_print<uint64_t>();
    const auto exp_3 = gen_rand_print<int16_t>();
    const auto exp_4 = gen_rand_print<int32_t>();
    const auto exp_5 = gen_rand_print<int64_t>();

    const std::string exp_str(base_str, sizeof(base_str));
    
    uint16_t rec_0;
    uint32_t rec_1;
    uint64_t rec_2;
    int16_t  rec_3;
    std::string rec_str;
    int32_t  rec_4;
    int64_t  rec_5;

    frame << exp_0;
    frame << exp_1;
    frame << exp_2;
    frame << exp_3;
    frame << exp_str; std::cout << "__ rand: wrote string[" << exp_str.size() << "]\n";
    frame << exp_4;
    frame << exp_5;

    frame >> rec_0; std::cout << "__ read: " << rec_0 << "\n";
    frame >> rec_1; std::cout << "__ read: " << rec_1 << "\n";
    frame >> rec_2; std::cout << "__ read: " << rec_2 << "\n";
    frame >> rec_3; std::cout << "__ read: " << rec_3 << "\n";
    frame >> rec_str; std::cout << "__ read string[" << rec_str.size() << "]\n";
    frame >> rec_4; std::cout << "__ read: " << rec_4 << "\n";
    frame >> rec_5; std::cout << "__ read: " << rec_5 << "\n";

    constexpr size_t expected_len = 
        sizeof(exp_0) +
        sizeof(exp_1) +
        sizeof(exp_2) +
        sizeof(exp_3) +
        sizeof(exp_4) +
        sizeof(exp_5) +
        sizeof(base_str) + sizeof(uint64_t); // flag

    if (exp_0 != rec_0) 
        throw std::runtime_error("Mismatch values for #0: " + std::to_string(exp_0) + " != " + std::to_string(rec_0));
    if (exp_1 != rec_1) 
        throw std::runtime_error("Mismatch values for #1: " + std::to_string(exp_1) + " != " + std::to_string(rec_1));
    if (exp_2 != rec_2) 
        throw std::runtime_error("Mismatch values for #2: " + std::to_string(exp_2) + " != " + std::to_string(rec_2));
    if (exp_3 != rec_3) 
        throw std::runtime_error("Mismatch values for #3: " + std::to_string(exp_3) + " != " + std::to_string(rec_3));
    if (exp_4 != rec_4) 
        throw std::runtime_error("Mismatch values for #4: " + std::to_string(exp_4) + " != " + std::to_string(rec_4));
    if (exp_5 != rec_5) 
        throw std::runtime_error("Mismatch values for #5: " + std::to_string(exp_5) + " != " + std::to_string(rec_5));

    if (exp_str != rec_str || exp_str.length() != rec_str.length() || rec_str.length() != (sizeof(base_str)))
        throw std::runtime_error("Mismatch values for strings! Sizes: "
            "exp:" + std::to_string(exp_str.length()) +
            "; rec:" + std::to_string(rec_str.length()) +
            "; orig:" + std::to_string(sizeof(base_str))
        );

    if (expected_len != frame.size())
        throw std::runtime_error("Mismatch size of frame: " + std::to_string(expected_len) + " != " + std::to_string(frame.size()));
}