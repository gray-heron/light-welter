
#include <boost/algorithm/string.hpp>
#include <boost/variant.hpp>
#include <cmrc/cmrc.hpp>
#include <fstream>

#include "config.h"
#include "scene.h"

CMRC_DECLARE(resources);

extern std::string S(glm::vec4 in);
extern std::string S(glm::vec3 in);

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
    if (type_id == typeid(glm::vec3))
    {
        glm::vec3 ret;
        std::istringstream iss(value);
        STRONG_ASSERT(iss >> ret.x >> ret.y >> ret.z);
        return ret;
    }
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
    STRONG_ASSERT(doc.load_buffer(config_file.begin(), config_file.size()),
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

std::vector<PointLight> Config::LoadRTC(std::string config_path)
{
    std::vector<PointLight> ret;
    std::ifstream infile(config_path);

    std::string line;
    int in_int1, in_int2;
    std::istringstream in_ss;

    STRONG_ASSERT(std::getline(infile, line));
    log_.Info() << "RTC comment: \"" << line << "\"";

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("scene",
                 GetOption<std::string>("rtc_dir") + boost::trim_right_copy(line));

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("target_file",
                 GetOption<std::string>("rtc_dir") + boost::trim_right_copy(line));

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("recursion", ParseValue(typeid(int), line));

    STRONG_ASSERT(std::getline(infile, line));
    in_ss = std::istringstream(line);
    STRONG_ASSERT(in_ss >> in_int1 >> in_int2);
    SetParameter("resx", in_int1);
    SetParameter("resy", in_int2);

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("camera_pos", ParseValue(typeid(glm::vec3), line));

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("camera_lookat", ParseValue(typeid(glm::vec3), line));

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("camera_up", ParseValue(typeid(glm::vec3), line));

    STRONG_ASSERT(std::getline(infile, line));
    SetParameter("fov_factor", ParseValue(typeid(float), line));

    while (std::getline(infile, line))
    {
        if (line == "")
            continue;

        PointLight pl;
        std::istringstream iss(line);
        char c;
        float x, y, z, r, g, b, i;

        STRONG_ASSERT(iss >> c >> x >> y >> z >> r >> g >> b >> i);
        STRONG_ASSERT(c == 'L')
        log_.Info() << c << x << y << z << r << g << b << i;
        pl.position = glm::vec3(x, y, z);
        pl.intensity_rgb = (glm::vec3(r, g, b) / 255.0f) * (i / 100.0f);
        ret.push_back(pl);
    }

    return ret;
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
            log_.Warning() << "Assuming " << current_argument
                           << " is an RTC config file.";

            std::string::size_type last_slash = current_argument.find_last_of("/");
            std::string rtc_dir;

            if (last_slash == std::string::npos)
                rtc_dir = ".";
            else if (last_slash == 0)
                rtc_dir = "/";
            else
                rtc_dir = current_argument.substr(0, last_slash);

            SetParameter("rtc_file", current_argument);
            SetParameter("rtc_dir", rtc_dir + "/");
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
            else if (static_cast<string>(child.attribute("type").as_string()) == "vec3")
                value = ParseValue(typeid(glm::vec3), child.text().as_string());
            else
                STRONG_ASSERT(0)
        }
        else
        {
            std::string wtf = child.name();
            STRONG_ASSERT(params_.find(child.name()) != params_.end());
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
        if (type_id == typeid(glm::vec3))
            value = S(boost::any_cast<glm::vec3>(param.second));
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