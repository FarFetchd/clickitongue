#ifndef FARFETCHD_SIMPLECONFIG_INCLUDEGUARD_H_
#define FARFETCHD_SIMPLECONFIG_INCLUDEGUARD_H_

#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

namespace farfetchd {

class ConfigReader
{
public:
  void parseStringBlob(std::string blob)
  {
    std::istringstream in_stream;
    in_stream.str(blob);
    parseStream(in_stream);
  }

  bool parseFile(std::string filename)
  {
    try
    {
      std::ifstream file_in(filename);
      if (!file_in)
        return false;
      parseStream(file_in);
    } catch (std::exception const& e) { return false; }
    return true;
  }

  std::optional<int> getInt(std::string key) const
  {
    if (auto it = items_.find(key); it != items_.end())
      try { return std::stoi(it->second); } catch (std::exception const& e) {}
    return std::nullopt;
  }

  std::optional<float> getFloat(std::string key) const
  {
    if (auto it = items_.find(key); it != items_.end())
      try { return std::stof(it->second); } catch (std::exception const& e) {}
    return std::nullopt;
  }

  std::optional<double> getDouble(std::string key) const
  {
    if (auto it = items_.find(key); it != items_.end())
      try { return std::stod(it->second); } catch (std::exception const& e) {}
    return std::nullopt;
  }

  std::optional<std::string> getString(std::string key) const
  {
    if (auto it = items_.find(key); it != items_.end())
      return it->second;
    return std::nullopt;
  }

private:
  void parseStream(std::istream& in_stream)
  {
    for (std::string line; std::getline(in_stream, line); )
    {
      auto delim = line.find(":");
      if (delim == std::string::npos)
        continue;
      auto comment = line.find("#");
      if (comment != std::string::npos && comment < delim)
        continue;
      auto start_val = line.find_first_not_of(" ", delim + 1);
      std::string key = line.substr(0, delim);
      std::string val = line.substr(start_val);
      items_[key] = val;
    }
  }
  std::unordered_map<std::string, std::string> items_;
};

} // namespace farfetchd

#endif // FARFETCHD_SIMPLECONFIG_INCLUDEGUARD_H_
