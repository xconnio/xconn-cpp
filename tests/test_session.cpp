#include <iostream>

#include "xconn_cpp/session.h"

int main() {
    Session session;

    std::cout << "=== Test 1: Simple CallRequest ===\n";
    session.Call("http://example.com/api")
        .Arg(42)
        .Arg("hello")
        .Arg(1.1)
        .Arg(true)
        .Kwarg("name", "Alice")
        .Kwarg("age", 30)
        .Do();

    std::cout << "=== Test 1 Complete ===\n";

    return 0;
}
