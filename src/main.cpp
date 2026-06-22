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
#include <set> 
#include <unordered_map>
#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>

static std::unordered_map<std::string,std::string> completion_script;
static std::vector<std::string> script_matches;
static std::vector<std::string> process;

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
  return line;
}

std::vector<std::string> get_path_executables(){
  std::set<std::string> results;

  char *path_env = getenv("PATH");
  if(!path_env) return {};

  std::stringstream ss(path_env);
  std::string dir;

  while(std::getline(ss,dir, ':')){
    for(const auto &entry: std::filesystem::directory_iterator(dir)){
      if(!entry.is_regular_file()){continue;}
      std::string name = entry.path().filename().string();

      if(access(entry.path().c_str(), X_OK) == 0){
        results.insert(name);
      }
    }
  }
  return std::vector<std::string>(results.begin(),results.end());
}

std::vector<std::string> get_all_commands(){
  static const char *builtins[] = {
    "echo",
    "exit",
    "type",
    "pwd",
    "cd",
    "complete",
    "jobs",
    nullptr
  };
  std::set<std::string> cmds;

  for(int i = 0; builtins[i];i++){
    cmds.insert(builtins[i]);
  }
  auto execs = get_path_executables();
  for(auto &e: execs){
    cmds.insert(e);
  }
  return std::vector<std::string>(cmds.begin(),cmds.end());
}

char *command_generator(const char *text, int state){
  static std::vector<std::string> matches;
  static int i;
  if(!state){
    matches.clear();
    i = 0;
    
    auto cmds = get_all_commands();
    for(const auto &cmd : cmds){
      if(cmd.rfind(text,0) == 0){
        matches.push_back(cmd);
      }
    }
  }
  if(i < matches.size()){
    return strdup(matches[i++].c_str());
  }
  return nullptr;
}

std::vector<std::string> run_completer_script(const std::string &script, const std::string &command, const std::string &current_word, const std::string &previous_word){
  std::vector<std::string> result;

  int pipefd[2];

  if(pipe(pipefd) == -1) return result;

  pid_t pid = fork();

  if(pid == -1){
    close(pipefd[0]);
    close(pipefd[1]);
    return result;
  }

  if(pid == 0){
    if(dup2(pipefd[1], STDOUT_FILENO) == -1){
      _exit(127);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    setenv("COMP_LINE", rl_line_buffer, 1);
    std::string point_str = std::to_string(rl_point);
    setenv("COMP_POINT", point_str.c_str(), 1);

    execl(script.c_str(), script.c_str(), command.c_str(), current_word.c_str(), previous_word.c_str(), (char*)nullptr);
    perror(script.c_str());
    _exit(127);
  }
  close(pipefd[1]);

  FILE* fp = fdopen(pipefd[0], "r");

  if(fp){
    char buffer[1024];
    while(fgets(buffer,sizeof(buffer),fp)){
      std::string s(buffer);

      while(!s.empty() && (s.back() == '\n' || s.back() == '\r')){
        s.pop_back();
      }

      if(!s.empty()){
        result.push_back(s);
      }

    }

    fclose(fp);
  }else{
    close(pipefd[0]);
  }
  waitpid(pid, nullptr, 0);
  return result;
}

char* script_generator(const char* text, int state){
  static size_t i;
  if(!state){
    i = 0;
  }
  while(i < script_matches.size()){
    return strdup(script_matches[i++].c_str());
  }
  return nullptr;
}

char **my_completion(const char *text, int start, int end){
  (void)end;

  if(start == 0){
    //rl_attempted_completion_over = 1;
    return rl_completion_matches(text,command_generator);
  }
  std::string line(rl_line_buffer, start);
  bool is_new_word = start > 0 && std::isspace((unsigned char) rl_line_buffer[start - 1]);
  auto words = tokenize(line);

  if(words.empty()){
    return nullptr;
  }

  std::string command = words[0];
  auto it = completion_script.find(command);

  if(it == completion_script.end()){
    return nullptr;
  }
  std::string current_word(text);
  std::string previous_word;

  if(!words.empty()){
    previous_word = words.back();
  }
  script_matches = run_completer_script(it->second, command, current_word, previous_word);

  if(script_matches.empty()){
    //rl_attempted_completion_over = 1;
    return nullptr;
  }

  rl_attempted_completion_over = 1;
  rl_completion_append_character = ' ';
  return rl_completion_matches(text,script_generator);
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  rl_attempted_completion_function = my_completion;
  rl_bind_key('\t', rl_complete);
  while(true){

    char* line = readline("$ " );
    if(!line) break;
    std::string input(line);
    if(*line) add_history(line);
    free(line);
    if(input == "exit"){
      break;
    }
    std::vector<std::string> wordcollector = tokenize(input);

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
    bool run_in_back = false;
    if(wordcollector.back() == "&"){
      run_in_back = true;
      wordcollector.pop_back();
      process.push_back(command);
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

    }else if(command == "exit"){
      return 0;
    }else if(wordcollector[0] == "type"){
      std::string path_env = std::getenv("PATH");
      std::stringstream ss_path(path_env);
      std::string path; //= path_env.substr(0,path_env.find(":")) + '/' + input.substr(input.find(" ")+1);
      bool pathExists = false;

      if(input.substr(input.find(" ")+1,4) == "echo"  || input.substr(input.find(" ")+1,4) == "exit" || input.substr(input.find(" ")+1,4) == "type" || input.substr(input.find(" ")+1,3) == "pwd" || input.substr(input.find(" ")+1,8) == "complete" || input.substr(input.find(" ")+1,4) == "jobs"){
        std::cout << input.substr(input.find(" ")+1) << " is a shell builtin" << std::endl;
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
    }else if(wordcollector[0] == "complete"){
    
      if(wordcollector.size() == 4 && wordcollector[1] == "-C"){
          completion_script[wordcollector[3]] = std::filesystem::absolute(wordcollector[2]).string();
      }else if(wordcollector.size() == 3 && wordcollector[1] == "-p"){
        auto it = completion_script.find(wordcollector[2]);
        if(it != completion_script.end()){
          std::cout << "complete -C '" << it->second << "' " << wordcollector[2] << "\n";
        }else{
          if(wordcollector.size() >= 3){
            std::cout << "complete: " << wordcollector[2] << ": no completion specification\n";
          }else{
            std::cout << "complete: invalid usage\n";
          }
          
        }
      }else if(wordcollector.size() == 3 && wordcollector[1] == "-r"){
        completion_script.erase(wordcollector[2]);
      }else{
        if(wordcollector.size() >= 3){
            std::cout << "complete: " << wordcollector[2] << ": no completion specification\n";
          }else{
            std::cout << "complete: invalid usage\n";
          }
      }
    }else if(wordcollector[0] == "jobs"){
      //implement jobs
      if(process.size() > 0){
//        std::cout << "[" << process.size() << "]+  Running                 ";
        for(int i = 0; i < process.size(); i++){
          std::cout << "[" << i+1 << "]";
          if(i == process.size()-1){
            std::cout << "+";
          }else if(i == process.size()-2){
            std::cout << "-";
          }
          std::cout << "  Running                 " << process[i] << "\n";
        }
  //      for(std::string i: process){
    //      std::cout << i;
      //  }
       // std::cout << "\n";
      }

    }else{
      std::vector<char*> c_args;
      for(auto &a : wordcollector){
        c_args.push_back(const_cast<char*>(a.c_str()));
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
        if(run_in_back){
          std::cout << "[" << process.size() << "] " << pid << "\n";
        }else{
          waitpid(pid, nullptr, 0);
        }

      }else{
        perror("fork");
      }
      
    }
  }

}