#pragma once
#include <string>
#include <cctype>

std::string cap(const std::string& s){ 
  if(s.empty()) return s; 
  auto t=s; 
  t[0]=std::toupper(t[0]); 
  return t; 
}

bool is_plural(const std::string& s){ 
  return !s.empty() && s.back()=='s'; 
}

std::string copula_for(const std::string& s){ 
  return is_plural(s) ? "are" : "is"; 
}

std::string say_isa(const std::string& x, const std::string& y){
  return cap(x) + " " + copula_for(x) + " " + y + ".";
}

std::string say_consumes(const std::string& x, const std::string& y, const std::string& v){
  // "Cats drink water."
  return cap(x) + " " + v + " " + y + ".";
}
