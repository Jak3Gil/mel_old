#pragma once
#include <string>
#include <cctype>

inline std::string norm(const std::string& s){
  std::string t; t.reserve(s.size());
  for (unsigned char c : s) {
    if (std::isalnum(c) || std::isspace(c)) t.push_back(std::tolower(c));
    // drop punctuation like '?', ',', '.' from node keys
  }
  // collapse spaces
  std::string u; u.reserve(t.size());
  bool sp=false;
  for (char c: t){ if(c==' '){ if(!sp){u.push_back(' '); sp=true;} } else {u.push_back(c); sp=false;} }
  if (!u.empty() && u.front()==' ') u.erase(u.begin());
  if (!u.empty() && u.back()==' ')  u.pop_back();
  return u;
}
