#include <Lunaris/socket.h>

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

bool test_ipv4();
bool test_ipv6();
bool test_anys();

namespace Lunaris {
    namespace Socket {
        extern int _get_family_from_ip_addr(const std::string& ip);
    }
}

namespace LS = Lunaris::Socket;


constexpr const char* ipv4_examples[] = {
    "0.0.0.0",
    "127.0.0.1",
    "255.255.255.255",
    "1.2.3.4",
    "12.45.113.6",
    "96.69.12.45"
};

constexpr const char* ipv6_examples[] = {
    "2001:0db8:85a3:0000:0000:8a2e:0370:7334",
    "2001:db8:85a3:0:0:8a2e:370:7334",
    "2001:af4d:335::ab3:412:645",
    "2001:ad:1:77ff:24a:9a::",
    "fe80::f2de:f1ff:fe14:1234",
    "fe80::1",
    "::1",
    "::"
};

constexpr const char* other_examples[] = {
    "256.256.256.256",
    "abobra.com",
    "12.34.56",
    "12.34.56.78.90",
    "lettuce",
    "jo.hn.97.45",
    "ab.0a.3d.df",
    "12.45.88.1:565",
    ":::3faa:90d",
    "2001:fe80:::"
};

int main()
{
    if (!test_ipv4()) {
        std::cout << "Failed on test IPV4!\n";
        return 1;
    }
    if (!test_ipv6()) {
        std::cout << "Failed on test IPV6!\n";
        return 1;
    }
    if (!test_anys()) {
        std::cout << "Failed on test ANY!\n";
        return 1;
    }

    return 0;
}


bool test_ipv4()
{
    constexpr auto assert_inet = [](int val) { return val == PF_INET; };
    bool success = true;

    std::cout << "Testing IPV4 scenarios...\n";

    for(const auto& i : ipv4_examples) {
        if (!assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : ipv6_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : other_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }

    return success;
}

bool test_ipv6()
{
    constexpr auto assert_inet = [](int val) { return val == PF_INET6; };
    bool success = true;

    std::cout << "Testing IPV6 scenarios...\n";

    for(const auto& i : ipv4_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : ipv6_examples) {
        if (!assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : other_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }

    return success;
}

bool test_anys()
{
    constexpr auto assert_inet = [](int val) { return val == PF_UNSPEC; };
    bool success = true;

    std::cout << "Testing ANY scenarios...\n";

    for(const auto& i : ipv4_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : ipv6_examples) {
        if (assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }
    for(const auto& i : other_examples) {
        if (!assert_inet(LS::_get_family_from_ip_addr(i))) {
            success = false;
            std::cout << "- " << i << " | FAILED\n";
        }
        else {
            std::cout << "- " << i << " | PASS\n";
        }
    }

    return success;
}
