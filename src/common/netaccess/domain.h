/**
 * @file domain.h
 * @brief ABAC (Attribute-Based Access Control) data model for netaccess.
 *
 * Implements the access model per technical specification TZ-NETACCESS-001:
 *   - actions instead of POSIX permissions: read/write/manage/connect/select/
 *     modify/print/access/admin;
 *   - role is an attribute of the subject (RBAC is a special case of ABAC);
 *   - resource carries a type;
 *   - access policies drive the decision; missing rule means deny by default.
 *
 * Documentation is generated with Doxygen in accordance with ESPD
 * (GOST 19.402-78).
 */

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace domain
{

/// Identifier of a subject or a resource.
using ID = uint64_t;

// ---------------------------------------------------------------------------
// Actions (permissions). Stored in the database as textual codes
// (see docs/database.md).
// ---------------------------------------------------------------------------

/**
 * @brief Actions (permissions) a subject can perform on a resource.
 */
enum class Action
{
    read,    ///< View/read a resource.
    write,   ///< Write to a resource.
    manage,  ///< Manage a resource (including granting rights).
    connect, ///< Establish a connection (server/VPN).
    select,  ///< Query data (database).
    modify,  ///< Modify data (database).
    print,   ///< Print.
    access,  ///< Access a service.
    admin,   ///< Full administration of a resource.
    any,     ///< Wildcard: any action in policies.
};

/**
 * @brief Converts an action to its textual code.
 *
 * @param[in] action Action to convert.
 * @return Textual code used on the wire and in the database.
 */
[[nodiscard]] inline std::string_view action_to_string(Action action)
{
    using namespace std::string_view_literals;
    switch (action)
    {
    case Action::read:
        return "read"sv;
    case Action::write:
        return "write"sv;
    case Action::manage:
        return "manage"sv;
    case Action::connect:
        return "connect"sv;
    case Action::select:
        return "select"sv;
    case Action::modify:
        return "modify"sv;
    case Action::print:
        return "print"sv;
    case Action::access:
        return "access"sv;
    case Action::admin:
        return "admin"sv;
    case Action::any:
        return "*"sv;
    }
    return "?"sv;
}

// ---------------------------------------------------------------------------
// Roles. A role is an attribute of the subject (role-based access is
// consistent with the attribute model: RBAC is a special case of ABAC).
// ---------------------------------------------------------------------------

/**
 * @brief Roles of the system.
 *
 * The role is treated as a subject attribute, not as a separate mechanism.
 */
enum class Role
{
    user,    ///< Regular user: view accessible resources.
    auditor, ///< Auditor: view the audit log (read-only).
    admin,   ///< Administrator: full management.
};

// ---------------------------------------------------------------------------
// Subject (user) with attributes.
// ---------------------------------------------------------------------------

/**
 * @brief Subject (user) with ABAC attributes.
 */
struct Subject
{
    ID id = 0;
    std::string username;
    std::string full_name;
    std::string department; ///< Department.
    Role role = Role::user;
    int clearance_level = 0; ///< Clearance level, 0..5.
    bool is_active = true;
};

// ---------------------------------------------------------------------------
// Object (network resource) with attributes.
// ---------------------------------------------------------------------------

/**
 * @brief Types of network resources.
 */
enum class ResourceType
{
    file_share,  ///< File share.
    database,    ///< Database.
    printer,     ///< Network printer.
    web_service, ///< Internal web service.
    server,      ///< Server.
    vpn,         ///< VPN tunnel.
};

/**
 * @brief Converts a resource type to its textual code.
 *
 * @param[in] type Resource type to convert.
 * @return Textual code used on the wire and in the database.
 */
[[nodiscard]] inline std::string_view resource_type_to_string(ResourceType type)
{
    using namespace std::string_view_literals;
    switch (type)
    {
    case ResourceType::file_share:
        return "file_share"sv;
    case ResourceType::database:
        return "database"sv;
    case ResourceType::printer:
        return "printer"sv;
    case ResourceType::web_service:
        return "web_service"sv;
    case ResourceType::server:
        return "server"sv;
    case ResourceType::vpn:
        return "vpn"sv;
    }
    return "?"sv;
}

/**
 * @brief Object (network resource) with ABAC attributes.
 */
struct Resource
{
    ID id = 0;
    std::string name;
    ResourceType type = ResourceType::file_share;
};

// ---------------------------------------------------------------------------
// Access policy (ABAC rule).
//
// A rule grants `action` on a resource when all of the following conditions
// hold:
//   - role_required       (if set): subject role must match;
//   - department_required (if set): subject department must match;
//   - min_clearance       (if set): subject clearance_level >= threshold;
//   - resource_type       (if set): resource type must match;
//   - subject_id          (if set): specific subject (GRANT_ACCESS);
//   - resource_id         (if set): specific resource (GRANT_ACCESS).
// With no matching rule, access is denied (deny by default).
// ---------------------------------------------------------------------------

/**
 * @brief Access policy (ABAC rule).
 */
struct Policy
{
    ID id = 0;
    std::string name;
    Action action = Action::any; ///< '*' means the action is not restricted.
    int priority = 0;
    bool enabled = true;
    std::optional<Role> role_required;
    std::optional<std::string> department_required;
    std::optional<int> min_clearance;
    std::optional<ResourceType> resource_type;
    std::optional<ID> subject_id;  ///< Specific subject (GRANT_ACCESS).
    std::optional<ID> resource_id; ///< Specific resource (GRANT_ACCESS).
};

// ---------------------------------------------------------------------------
// Evaluation result.
// ---------------------------------------------------------------------------

/**
 * @brief Result of policy evaluation.
 */
struct Decision
{
    std::string reason; ///< Reason of denial/allowance.
    bool allowed = false;
};

// ---------------------------------------------------------------------------
// ABAC decision function.
//
// Pure ABAC model: the decision is made by a policy only.
// Evaluation order:
//   1) the subject must be active;
//   2) enabled policies whose action matches (or is '*') and whose conditions
//      (role, department, clearance level, resource type, specific
//      subject/resource) hold are selected;
//   3) the policy with the highest priority is applied;
//   4) no matching policy -> deny (deny by default).
// ---------------------------------------------------------------------------

/**
 * @brief Checks whether a policy matches the subject and the resource.
 *
 * @param[in] subject  Subject being checked.
 * @param[in] resource Resource being accessed.
 * @param[in] policy   Policy to evaluate.
 * @return true if all policy conditions hold, false otherwise.
 */
[[nodiscard]] inline bool policy_matches(const Subject& subject, const Resource& resource, const Policy& policy)
{
    if (policy.role_required && *policy.role_required != subject.role)
    {
        return false;
    }
    if (policy.department_required && *policy.department_required != subject.department)
    {
        return false;
    }
    if (policy.min_clearance && subject.clearance_level < *policy.min_clearance)
    {
        return false;
    }
    if (policy.resource_type && *policy.resource_type != resource.type)
    {
        return false;
    }
    if (policy.subject_id && *policy.subject_id != subject.id)
    {
        return false;
    }
    if (policy.resource_id && *policy.resource_id != resource.id)
    {
        return false;
    }
    return true;
}

/**
 * @brief Evaluates an ABAC access request.
 *
 * @param[in] subject  Subject requesting access.
 * @param[in] resource Resource being accessed.
 * @param[in] action   Requested action.
 * @param[in] policies Candidate policies.
 * @return Decision with the result and the reason.
 */
[[nodiscard]] inline Decision evaluate_policy(const Subject& subject, const Resource& resource, Action action,
                                              const std::vector<Policy>& policies)
{
    if (!subject.is_active)
    {
        return Decision{false, "account_inactive"};
    }

    // Select enabled policies whose conditions hold, with the highest priority.
    const Policy* best = nullptr;
    for (const auto& policy : policies)
    {
        if (!policy.enabled)
        {
            continue;
        }
        if (policy.action != Action::any && policy.action != action)
        {
            continue;
        }
        if (!policy_matches(subject, resource, policy))
        {
            continue;
        }
        if (best == nullptr || policy.priority > best->priority)
        {
            best = &policy;
        }
    }

    if (best == nullptr)
    {
        return Decision{false, "no_matching_policy"};
    }

    return Decision{true, "policy:" + best->name};
}

} // namespace domain
