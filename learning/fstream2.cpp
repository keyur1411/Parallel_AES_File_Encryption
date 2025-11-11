#include<fstream>
#include<iostream>
#include<sstream>

int main(){
  // std::ofstream outf{"food.txt"};
  // std::string keyur;
  // std::cin>>keyur;
  // outf<<keyur;

  std::stringstream outf("1 2");
  int a;
  int b;
  outf>>a>>b;
  std::cout<<a+b<<'\n';
}