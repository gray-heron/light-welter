
#include <boost/variant.hpp>
#include <cmrc/cmrc.hpp>

#include "config.h"

CMRC_DECLARE(resources);

using std::string;

const string NAME_PREFIX = "--";
const char NAME_VALUE_SEPARATOR = '=';

boost::any ParseValue(const std::type_info &type_id, std::string value)
{
    if (type_id == typeid(string))
        return value;
    if (type_id == typeid(int))
        return std::stoi(value);
    if (type_id == typeid(float))
        return std::stof(value);
    if (type_id == typeid(double))
        return std::stod(value);
    if (type_id == typeid(bool))
    {
        if (value == "true")
            return true;
        else if (value == "false")
            return false;

        return static_cast<bool>(std::stoi(value));
    }

    throw Exception((string) "Unrecognized type: " + type_id.name());
}

Config::Config()
{
    pugi::xml_document doc;
    auto fs = cmrc::resources::get_filesystem();
    auto config_file = fs.open("res/default_configuration.xml");
    ASSERT(doc.load_buffer(config_file.begin(), config_file.size()),
           "Couldn't parse default configuration!");
    LoadXMLConfig(doc);
}

void Config::Load(std::string config_path)
{
    pugi::xml_document doc;

    if (doc.load_file(config_path.c_str()))
        LoadXMLConfig(doc);
    else
        log_.Error() << "Couldn't parse configuration";
}

void Config::Load(int argc, char **argv)
{
    for (int arg_i = 1; arg_i < argc; arg_i++)
    {
        string current_argument(argv[arg_i]), current_name, current_value;
        std::map<std::string, boost::any>::iterator param_entry;
        unsigned int cursor = NAME_PREFIX.length();

        if (NAME_PREFIX != "" && current_argument.find(NAME_PREFIX) != 0)
        {
            log_.Error() << "Wrong prefix on argument: " << current_argument << "!";
            continue;
        }

        while (cursor < current_argument.length() &&
               current_argument[cursor] != NAME_VALUE_SEPARATOR)
        {
            current_name += current_argument[cursor];
            cursor++;
        }

        if (cursor == current_argument.length())
        {
            log_.Error() << "No separator on argument: " << current_argument << "!";
            continue;
        }

        if ((param_entry = params_.find(current_name)) == params_.end())
        {
            log_.Error() << "Argument not recognized: " << current_name << "!";
            continue;
        }

        for (cursor += 1; cursor < current_argument.length(); cursor++)
            current_value += current_argument[cursor];

        params_[current_name] = ParseValue(param_entry->second.type(), current_value);
    }
}

void Config::LoadXMLConfig(pugi::xml_document &doc)
{
    // FIXME: error handling
    for (auto child : doc.root().child("configuration").children())
    {
        boost::any value;

        if (!child.attribute("type").empty())
        {
            if (static_cast<string>(child.attribute("type").as_string()) == "string")
                value = ParseValue(typeid(string), child.text().as_string());
            else if (static_cast<string>(child.attribute("type").as_string()) == "int")
                value = ParseValue(typeid(int), child.text().as_string());
            else if (static_cast<string>(child.attribute("type").as_string()) == "float")
                value = ParseValue(typeid(float), child.text().as_string());
            else if (static_cast<string>(child.attribute("type").as_string()) == "double")
                value = ParseValue(typeid(double), child.text().as_string());
            else if (static_cast<string>(child.attribute("type").as_string()) == "bool")
                value = ParseValue(typeid(bool), child.text().as_string());
        }
        else
        {
            std::string wtf = child.name();
            ASSERT(params_.find(child.name()) != params_.end());
            value = ParseValue(params_[child.name()].type(), child.text().as_string());
        }

        params_[child.name()] = value;
    }
}

void Config::SetParameter(std::string name, boost::any val) { params_[name] = val; }

void Config::DumpSettings()
{
    for (const auto &param : params_)
    {
        string value;
        auto &type_id = param.second.type();
        if (type_id == typeid(string))
            value = boost::any_cast<string>(param.second);
        if (type_id == typeid(int))
            value = std::to_string(boost::any_cast<int>(param.second));
        if (type_id == typeid(float))
            value = std::to_string(boost::any_cast<float>(param.second));
        if (type_id == typeid(double))
            value = std::to_string(boost::any_cast<double>(param.second));
        if (type_id == typeid(bool))
        {
            if (boost::any_cast<bool>(param.second))
                value = "true";
            else
                value = "false";
        }

        log_.Info() << "Param \"" << param.first << "\" = " << value;
    }
}