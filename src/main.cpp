#include <iostream>
#include <string>
#include <unistd.h>

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
    }else if(input.substr(0,input.find(" ")) == "type"){
      std::string path_env = std::getenv("PATH");
      std::string path = path_env.substr(0,path_env.find(":")) + '/' + input.substr(input.find(" ")+1);
      if(input.substr(input.find(" ")+1,4) == "echo"){
        std::cout << "echo is a shell builtin" << std::endl;
      }else if(input.substr(input.find(" ")+1,4) == "exit"){
        std::cout << "exit is a shell builtin" << std::endl;
      }else if(input.substr(input.find(" ")+1,4) == "type"){
        std::cout << "type is a shell builtin" << std::endl;
      }else if(!access(path.c_str(),X_OK)){
        std::cout << input.substr(input.find(" ")+1) << " is " << path << std::endl;
      }else{
        std::cout << input.substr(input.find(" ")+1) << ": not found" << std::endl;
        std::cout << path << endl;
      }
    }else{
      std::cout << input << ": command not found" << std::endl;
    }
  }

}

// check if command is built iin
// if it is print 