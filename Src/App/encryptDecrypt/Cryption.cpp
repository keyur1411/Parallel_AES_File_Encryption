#include "Cryption.hpp"
#include "../Processes/Task.hpp"
#include "../FileHandling/ReadEnv.hpp"
#include <string>
#include <ctime>
#include <iomanip>

int excecuteCryption(const std::string &taskData){
  Task task = Task::fromString(taskData);
  ReadEnv env;
  const std::string env_Key = env.getTheEnv();
  const int key = std::stoi(env_Key);
  if(task.action == Action::ENCRYPT){
    char ch;
    while(task.f_stream.get(ch)){
      ch = (ch+key)%256;
      task.f_stream.seekp(-1,std::ios::cur);
      task.f_stream.put(ch);
    }
    task.f_stream.close();
  }else {
    char ch;
    while(task.f_stream.get(ch)){
      ch = (ch-(key%256) +256)%256;
      task.f_stream.seekp(-1,std::ios::cur);
      task.f_stream.put(ch);
    }
    task.f_stream.close();
  }
  std::time_t t = std::time(nullptr);
  std::tm* now = std::localtime(&t);
  std::cout<<"Exiting the Encryption/Decryption : "<<std::put_time(now,"%Y-%m-%d %H:%M:%S")<<std::endl;
  return 0;
}
