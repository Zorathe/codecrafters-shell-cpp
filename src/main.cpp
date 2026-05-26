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
      std::cout << input.substr(5) << std::endl;
    }else if(input.substr(0,input.find(" ")) == "type")){
      if(input.substr(input.find(" "),4 == "echo")){
        std::cout << "echo is a shell builtin" << std::endl;
      }else if(input.substr(input.find(" "),4) == "exit")){
        std::cout << "exit is a shell builtin" << std::endl;
      }else{
        std::cout << input.substr(input.find(" ")) << ": not found" << std::endl;
      }
    }else{
      std::cout << input << ": command not found" << std::endl;
    }
  }

}