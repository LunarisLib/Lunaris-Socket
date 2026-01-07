#pragma once

#include <string>
#include <random>
#include <string.h>

inline std::string generate_random_string(size_t size)
{
    constexpr char characters[] = "0123456789abcdefhijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-=+-*/@!~\\/$% ";
    constexpr size_t characters_len = strlen(characters);

    std::string str(size, '\0');
    std::random_device rd; 
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distrib(0, characters_len-1);

    for(auto& ch : str) {
        ch = characters[distrib(gen)];
    }

    return str;
}

inline uint16_t generate_random_port() {
    std::random_device rd; 
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distrib(0, 999);

    return 55000 + distrib(gen);
}

inline uint32_t generate_random_in_range(uint32_t low, uint32_t high) {
    std::random_device rd; 
    std::mt19937 gen(rd());

    std::uniform_int_distribution<> distrib(low, high);

    return distrib(gen);
}

template<typename CLIENT>
inline bool send_and_check_auto(CLIENT& cli, const std::string& str) {
    return cli.send(str.data(), str.size()) == str.size();
};

template<typename CLIENT>
inline bool recv_tcp_and_check_auto(CLIENT& cli, const std::string& str) {
    const auto siz = str.size();
    std::string buf(siz, '\0');
    const auto res = cli.recv_autowait(buf.data(), siz);
    return res == siz && res == buf.size() && memcmp(buf.data(), str.data(), siz) == 0;
};

template<typename CLIENT>
inline bool recv_udp_and_check_auto(CLIENT& cli, const std::string& str) {
    const auto siz = str.size();
    std::string buf(siz, '\0');
    const auto res = cli.recv(buf.data(), siz);
    return res == siz && res == buf.size() && memcmp(buf.data(), str.data(), siz) == 0;
};