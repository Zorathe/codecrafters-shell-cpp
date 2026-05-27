#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
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
      std::stringstream ss_path(path_env);
      std::string path; //= path_env.substr(0,path_env.find(":")) + '/' + input.substr(input.find(" ")+1);
      bool pathExists = false;

      if(input.substr(input.find(" ")+1,4) == "echo"  || input.substr(input.find(" ")+1,4) == "exit" || input.substr(input.find(" ")+1,4) == "type"){
        std::cout << input.substr(input.find(" ")+1,4) << " is a shell builtin" << std::endl;
        pathExists = true;
      }
      }else{
        while(std::getline(ss_path,path, ':')){
          std::string full_path = path + '/' + input.substr(input.find(" ")+1);
          if(!access(full_path.c_str(),X_OK)){
            std::cout << input.substr(input.find(" ")+1) << " is " << full_path << std::endl;
            pathExists = true;
            break;
          }
        }
      } 
      if(!pathExists){
        std::cout << input.substr(input.find(" ")+1) << ": not found" << std::endl;
        std::cout << input.substr(0,10) << "check this out" << std::endl;
      }
    }else if(input.substr(0,10) == "custom_exe"){
      std::cout << "this worked" << std::endl;
      int argCount = 0;
      std::stringstream args;
      while(std::getline(args,input, ' ')){
        argCount++;
      }
      std::cout << "Program was passed " << argCount << " args (including program name)." << std::endl;
      pathExists == true;
    }else{
      std::cout << input << ": command not found" << std::endl;
      std::cout << (input.substr(0,10)  == "custom_exe") << "not working this out" << std::endl;
    }
  }

}

// determine if custom_exe is an executable in PATH
// execute with three arguments: custom_exe, arg1, and arg2