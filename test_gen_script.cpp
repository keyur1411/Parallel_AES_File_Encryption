#include <fstream>
#include <iostream>
#include <string>
#include <random>


int main(){
    for(int i=0;i<10;i++){
      std::string file_path = "Test/test"+std::to_string(i)+".txt";
      std::ofstream file_stream(file_path,std::ios::out);
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dist(0,255);
      for(int i=0;i<200;i++){
        int ran = dist(gen);
        ran%=26;
        file_stream<<char(ran+'a');
      }
    }
    return 0;
}