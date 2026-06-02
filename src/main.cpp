#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <regex>



int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  
  while(true){
    std::cout << "$ ";
    std::string input;
    std::getline(std::cin, input);
    if(input == "exit"){
      break;
    }
    enum State {NORMAL, SINGLE, DOUBLE};
    State state = NORMAL;
    if(input.substr(0,input.find(" ")) == "echo"){
      //implement echo with single quotes
      bool quoteOpened = false;
      std::string text = input.substr(5);
      std::string word = "";
      std::vector<std::string> line; 
      for(int i = 0; i < text.length();i++){
        if(state != SINGLE && text[i] == '\\'){
          if(i + 1 < text.size()){
            word += text[i+1];
            i++;
          }
        }else if(text[i] == '\'' && state != DOUBLE){
          state = (state == SINGLE) ? NORMAL : SINGLE;
        }else if(text[i] == '\"' && state != SINGLE){
          state = (state == DOUBLE) ? NORMAL : DOUBLE;
        }else if(text[i] == ' ' && state == NORMAL){
          if(!word.empty()){
            line.push_back(word);
            word.clear();
          }
        }else{
            word += text[i];
        }
    
      }

      if(!word.empty()){
        line.push_back(word);
      }
      for(std::string s: line){
        std::cout << s << " ";
      }
      std::cout << std::endl;
    }else if(input.substr(0,input.find(" ")) == "type"){
      std::string path_env = std::getenv("PATH");
      std::stringstream ss_path(path_env);
      std::string path; //= path_env.substr(0,path_env.find(":")) + '/' + input.substr(input.find(" ")+1);
      bool pathExists = false;

      if(input.substr(input.find(" ")+1,4) == "echo"  || input.substr(input.find(" ")+1,4) == "exit" || input.substr(input.find(" ")+1,4) == "type" || input.substr(input.find(" ")+1,3) == "pwd" ){
        std::cout << input.substr(input.find(" ")+1,4) << " is a shell builtin" << std::endl;
        pathExists = true;
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
      }
    }else if(input.substr(0,10) == "custom_exe"){
      std::system(input.c_str());
    
    }else if(input == "pwd"){
      std::cout << std::filesystem::current_path().string() << std::endl;
    
    }else if(input.substr(0,2) == "cd"){
        std::string p = input.substr(input.find(" ")+1);
        p = std::regex_replace(p, std::regex("~"), std::getenv("HOME"));
      if(chdir(p.c_str()) != 0){
        std::cout << "cd: " << input.substr(input.find(" ")+1) << ": No such file or directory"<< std::endl;    
      }
    }else if(input.substr(0,3) == "cat" || input[0] == '\'' || input[0] == '\"'){
      std::system(input.c_str());
    }else{
      std::cout << input << ": command not found" << std::endl;
    }
  }

}