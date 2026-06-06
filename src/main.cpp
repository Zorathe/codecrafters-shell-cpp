#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <regex>
#include <fcntl.h>
#include <sys/wait.h>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  
  while(true){
    std::cout << "$ " << std::flush;
    std::string input;
    std::getline(std::cin, input);
    if(input == "exit"){
      break;
    }
    std::string part;
    std::vector<std::string> wordcollector;
    std::stringstream ss(input);
    while(getline(ss, part, ' ')){
      wordcollector.push_back(part);
    }
    std::string file;
    bool writefile = false;
    int saved_stdout = -1;
    //    int saved_stdout = dup(STDOUT_FILENO);
    if(wordcollector.size() > 2 && (wordcollector[wordcollector.size()-2] == ">" || wordcollector[wordcollector.size()-2] == "1>")){
    // implement the > operator
      writefile = true;
      file = wordcollector.back();
      wordcollector.pop_back();
      wordcollector.pop_back();
      // pid_t pid = fork();
      // if(pid == 0){
      //   if(writefile){
      //     int file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
      //     dup2(file_desc,STDOUT_FILENO);
      //     close(file_desc);
      //   }
    
      // }
    // read the > or 1> 
    // then open file
    }

    std::string command;
    for(int i = 0; i < wordcollector.size();i++){
      if(i) command += " ";
      command += wordcollector[i];
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
      std::system(command.c_str());
    
    }else if(input == "pwd"){
      std::cout << std::filesystem::current_path().string() << std::endl;
    
    }else if(input.substr(0,2) == "cd"){
        std::string p = input.substr(input.find(" ")+1);
        p = std::regex_replace(p, std::regex("~"), std::getenv("HOME"));
      if(chdir(p.c_str()) != 0){
        std::cout << "cd: " << input.substr(input.find(" ")+1) << ": No such file or directory"<< std::endl;    
      }
    /*}else if(input.substr(0,3) == "cat" || input[0] == '\'' || input[0] == '\"'){
      std::system(command.c_str());

      //std::cout << "entered cat: " << input << std::endl;
    */}else{
      std::vector<char*> c_args;
      for(auto &a : wordcollector){
        c_args.push_back(&a[0]);
      }
      c_args.push_back(nullptr);
      pid_t pid = fork();
      if(pid == 0){
        if(writefile){
          int file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if(file_desc == -1){
            perror("open");
            exit(1);
          }
          if(dup2(file_desc, STDOUT_FILENO) == -1){
            perror("dup2");
            exit(1);
          }
          close(file_desc);
        }
        execvp(c_args[0], c_args.data());
        std::cout << command << ": command not found\n";
        perror("execvp")
        exit(1);
      }else{
        wait(nullptr);
      }
      
    }
    saved_stdout = dup(STDOUT_FILENO);
    if(writefile){
      dup2(saved_stdout,STDOUT_FILENO);
      close(saved_stdout);
    }
  }

}