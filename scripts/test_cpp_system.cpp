#include <cstdlib>
#include <string>
#include <iostream>
int main() {
    std::string vault_path = "data/muthr_vaults/Pacific 1941";
    std::string yoink_dir = vault_path + "\\yoink";
    std::string cmd = "if not exist \"" + yoink_dir + "\" mkdir \"" + yoink_dir + "\" && start \"\" cmd /k \"cd /d \\\"" + yoink_dir + "\\\" && yoink\"";
    std::cout << "CMD: " << cmd << std::endl;
    int res = std::system(cmd.c_str());
    std::cout << "Result: " << res << std::endl;
    return 0;
}