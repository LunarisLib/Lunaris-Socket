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


    if (
        exp_0 != rec_0 ||
        exp_1 != rec_1 ||
        exp_2 != rec_2 ||
        exp_3 != rec_3 ||
        exp_4 != rec_4 ||
        exp_5 != rec_5 ||
        exp_str != rec_str || exp_str.length() != rec_str.length() || rec_str.length() != (sizeof(base_str))
    ) throw std::runtime_error("Mismatch on value expected vs obtained!");
}