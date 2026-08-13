#include <filesystem>
#include <map>
#include <string>
#include <variant>
#include <vector>

// ABAC (Attribute-Based Access Control)
namespace models
{

namespace fs = std::filesystem;
using ID = uint64_t;
using AttributeValue = std::variant<int, std::string, bool, fs::perms>;

template <typename T>
concept ValidAttributeType =
    std::same_as<T, int> || std::same_as<T, std::string> || std::same_as<T, bool> || std::same_as<T, fs::perms>;

struct Subject
{
    std::string id;
    std::map<std::string, AttributeValue, std::less<>> attributes;

    template <ValidAttributeType T> [[nodiscard]] T get_attribute(std::string_view key, T default_value) const
    {
        auto it = attributes.find(key);
        if (it != attributes.end() && std::holds_alternative<T>(it->second))
        {
            return std::get<T>(it->second);
        }
        return default_value;
    }
};

struct Resource
{
    std::string name;
    std::filesystem::perms required_perms;
};

[[nodiscard]] bool evaluate_policy(const Subject& subject, const Resource& resource)
{
    int clearance = subject.get_attribute<int>("clearance_level", 0);
    if (clearance < 3)
    {
        return false;
    }

    auto user_perms = subject.get_attribute<std::filesystem::perms>("allowed_perms", std::filesystem::perms::none);

    return (user_perms & resource.required_perms) == resource.required_perms;
}

} // namespace models
