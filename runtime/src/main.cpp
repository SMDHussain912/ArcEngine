#include <iostream>
#include "ArcEngine/Core/Version.h"

int main() {
    std::cout << "ArcEngine v" << Arc::GetVersion() << std::endl;
    std::cout << "M0 Foundation OK. Next: M1 Window (GLFW)." << std::endl;
    return 0;
}
