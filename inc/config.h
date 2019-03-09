
#pragma once

#include <boost/any.hpp>
#include <exceptions.h>
#include <map>
#include <pugixml.hpp>
#include <string>

class Config
{
  private:
    Config();
    std::map<std::string, boost::any> params_;
    void LoadXMLConfig(pugi::xml_document &doc);

    Log log_{"Configuration"};

  public:
    Config(Config const &) = delete;
    void operator=(Config const &) = delete;

    static Config &inst()
    {
        static Config instance;
        return instance;
    }

    void Load(std::string config_path);
    void Load(int argc, char **argv);

    void SetParameter(std::string name, boost::any val);
    void DumpSettings();

    template <typename T> T GetOption(std::string name)
    {
        auto val = params_.find(name);
        ASSERT(val != params_.end(), "No such option: " + name);
        ASSERT(val->second.type() == typeid(T),
               "Requested option " + name + " with type " + typeid(T).name() +
                   " but got " + val->second.type().name());
        return boost::any_cast<T>(val->second);
    }
};