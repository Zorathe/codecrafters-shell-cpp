#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <filesystem>
#include <regex>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

std::vector<std::string> tokenize(const std::string &input){
  //bool quoteOpened = false;
  //std::string text = input.substr(5);
  enum State {NORMAL, SINGLE, DOUBLE};
  State state = NORMAL;
  std::string word = "";
  std::vector<std::string> line; 
  for(int i = 0; i < input.length();i++){
    if(state != SINGLE && input[i] == '\\'){
      if(i + 1 < input.size()){
        word += input[i+1];
        i++;
      }
    }else if(input[i] == '\'' && state != DOUBLE){
      state = (state == SINGLE) ? NORMAL : SINGLE;
    }else if(input[i] == '\"' && state != SINGLE){
      state = (state == DOUBLE) ? NORMAL : DOUBLE;
    }else if(input[i] == ' ' && state == NORMAL){
      if(!word.empty()){
        line.push_back(word);
        word.clear();
      }
    }else{
        word += input[i];
    }
  }

  if(!word.empty()){
    line.push_back(word);
  }
  // for(std::string s: line){
  //   std::cout << s << " ";
  // }
  // std::cout << std::endl;
  return line;
}
  static const char *builtins[] = {
    "echo",
    "exit",
    "type",
    "pwd",
    "cd",
    nullptr
  };
char *command_generator(const char *text, int state){
  static int i;
  static size_t len;
  if(!state){
    i = 0;
    len = strlen(text);
  }
  while(builtins[i]){
    const char *cmd = builtins[i++];
    if(strcmp(cmd,text) == 0 && strcmp(len,text) == 0 && strcmp(cmd,len) == 0)
      return strdup(cmd);
  }
  return nullptr;
}

char **my_completion(const char *text, int start, int end){
  (void)start;
  (void)end;
  return rl_completion_matches(text,command_generator);
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;


  rl_attempted_completion_function = my_completion;
  rl_bind_key('\t', rl_complete);
  
  while(true){
    // std::cout << "$ " << std::flush;
    // std::string input;
    // std::getline(std::cin, input);
    char* line = readline("$ " );
    if(!line) break;
    std::string input(line);
    if(*line) add_history(line);
    free(line);
    if(input == "exit"){
      break;
    }
    std::string part;
    std::vector<std::string> wordcollector = tokenize(input);
    //std::stringstream ss(input);
    // while(getline(ss, part, ' ')){
    //   wordcollector.push_back(part);
    // }
    if(wordcollector.empty()) continue;
    std::string file;
    bool redirect = false;
    std::string redirect_type;
    if(wordcollector.size() > 2 && (wordcollector[wordcollector.size()-2] == ">" || wordcollector[wordcollector.size()-2] == "1>" || wordcollector[wordcollector.size()-2] == "2>" || wordcollector[wordcollector.size()-2] == ">>" || wordcollector[wordcollector.size()-2] == "1>>" || wordcollector[wordcollector.size()-2] == "2>>")){
    // implement the > operator
      redirect = true;
      redirect_type = wordcollector[wordcollector.size()-2];
      file = wordcollector.back();
      wordcollector.pop_back();
      wordcollector.pop_back();
    }

    std::string command;
    for(int i = 0; i < wordcollector.size();i++){
      if(i) command += " ";
      command += wordcollector[i];
    }


    if(wordcollector[0] == "echo"){
      
      int saved_stdout = -1;
      int saved_stderr = -1;

      if(redirect){
        //saved_stdout = dup(STDOUT_FILENO);
        int file_desc;
        if(redirect_type == ">>" || redirect_type == "1>>" || redirect_type == "2>>"){
          file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        }else{
          file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        if(file_desc == -1){
          perror("open");
          continue;
        }
        if(redirect_type == "2>" || redirect_type == "2>>"){
          saved_stderr = dup(STDERR_FILENO);
          dup2(file_desc, STDERR_FILENO);
        }else{
          saved_stdout = dup(STDOUT_FILENO);
          dup2(file_desc, STDOUT_FILENO);
          
        }
        close(file_desc);
      }
      for(int i = 1; i < wordcollector.size();i++){
        std::cout << wordcollector[i];
        if(i + 1 < wordcollector.size()) std::cout << " ";
      }
      std::cout << std::endl;

      if(redirect){
        std::cout.flush();
        std::cerr.flush();
        if(redirect_type == "2>" || redirect_type == "2>>"){
          dup2(saved_stderr, STDERR_FILENO);
          close(saved_stderr);
        }else{
          dup2(saved_stdout, STDOUT_FILENO);
          close(saved_stdout);
        }
        
      }
      continue;
      //implement echo with single quotes

    }else if(command == "exit"){
      return 0;
    }else if(wordcollector[0] == "type"){
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
    }/*else if(input.substr(0,10) == "custom_exe"){
      std::system(command.c_str());
    
    }*/else if(input == "pwd"){
      std::cout << std::filesystem::current_path().string() << std::endl;
    
    }else if(wordcollector[0] == "cd"){
        std::string p = input.substr(input.find(" ")+1);
        p = std::regex_replace(p, std::regex("~"), std::getenv("HOME"));
      if(chdir(p.c_str()) != 0){
        std::cout << "cd: " << input.substr(input.find(" ")+1) << ": No such file or directory"<< std::endl;    
      }
    /*}else if(input.substr(0,3) == "cat" || input[0] == '\'' || input[0] == '\"'){
      std::system(command.c_str());

      //std::cout << "entered cat: " << input << std::endl;
    */
    // }else if(command == '\t'){
    //   if(command == "ech"){
    //     command = "echo ";
    //     std::cout << "o " << std::flush;
    //   }else if(command == "exi"){
    //     command = "exit ";
    //     std::cout << "t " << std::flush;
    //   }else{
    //     std::cout << "\a" << std::flush;
    //   }
    }else{
      std::vector<char*> c_args;
      for(auto &a : wordcollector){
        c_args.push_back(&a[0]);
      }
      c_args.push_back(nullptr);
      pid_t pid = fork();
      if(pid == 0){
        if(redirect){
        int file_desc;
        if(redirect_type == ">>" || redirect_type == "1>>" || redirect_type == "2>>"){
          file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
        }else{
          file_desc = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
          if(file_desc == -1){
            perror("open");
            exit(1);
          }
        if(redirect_type == "2>" || redirect_type == "2>>"){
            if(dup2(file_desc, STDERR_FILENO) == -1){
              perror("dup2");
              exit(1);
            }
          }else if(dup2(file_desc, STDOUT_FILENO) == -1){
            perror("dup2");
            exit(1);
          }

          close(file_desc);
        }
        execvp(c_args[0], c_args.data());
        std::cerr << c_args[0] << ": command not found\n";
        exit(127);
      }else if(pid > 0){
        waitpid(pid, nullptr, 0);
      }else{
        perror("fork");
      }
      
    }
  }

}