#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // TODO: Uncomment the code below to pass the first stage
  while(true){
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);
    if(input == "exit"){
      break;
    }
    if(input.substr(0,input.find(" ")) == "echo"){
      std::cout << input.substr(5,input.size()-4) << std::endl;
      break;
    }
    std::cout << input << ": command not found" << std::endl;
  }

}