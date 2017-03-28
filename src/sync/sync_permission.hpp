////////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#ifndef REALM_OS_SYNC_PERMISSION_HPP
#define REALM_OS_SYNC_PERMISSION_HPP

#include "results.hpp"

namespace realm {

class Permissions;
class SyncUser;
class Object;

// Permission object used to represent a user permission
struct Permission {
    // The path of the Realm to which this permission pertains.
    std::string path;

    // A permission encapsulates a single access level.
    // Each level includes all the capabilities of the level
    // above it (for example, 'write' implies 'read').
    enum class AccessLevel {
        None,
        Read,
        Write,
        Admin,
    };
    AccessLevel access;

    // Condition is a userId or a KeyValue pair
    // Other conditions may be supported in the future
    struct Condition {
        enum class Type { UserId, KeyValue };
        Type type;

        union {
            std::string user_id;
            std::pair<std::string, std::string> key_value;
        };

        Condition(std::string id) : type(Type::UserId), user_id(std::move(id)) {}

        Condition& operator=(const Condition&);
        Condition(const Condition &c);
        ~Condition();
    };
    Condition condition;
};

class PermissionResults {
friend class Permissions;
public:
    // The number of permissions represented by this PermissionResults.
    size_t size();

    // Get the permission for the given index
    // Throws OutOfBoundsIndexException if index >= size()
    const Permission get(size_t index);

    // Create an async query from this Results
    // The query will be run on a background thread and delivered to the callback,
    // and then rerun after each commit (if needed) and redelivered if it changed
    NotificationToken async(std::function<void(std::exception_ptr)> target)
    {
        return m_results.async(target);
    }

    // Create a new instance by further filtering or sorting this instance.
    PermissionResults filter(Query&& q) const;

private:
    PermissionResults(Results results)
    : m_results(std::move(results))
    {}

    Results m_results;
};

class Permissions {
public:
    // Consumers of these APIs need to pass in a method which creates a Config with the proper
    // SyncConfig and associated callbacks, as well as the path and other parameters.
    using ConfigMaker = std::function<Realm::Config(std::shared_ptr<SyncUser>, std::string url)>;

    // Asynchronously retrieve the permissions for the provided user.
    static void get_permissions(std::shared_ptr<SyncUser>,
                                std::function<void(util::Optional<PermissionResults>, std::exception_ptr)>,
                                const ConfigMaker&);

    // Callback used to monitor success or errors when changing permissions
    // `exception_ptr` is null_ptr on success
    using PermissionChangeCallback = std::function<void(std::exception_ptr)>;

    // Set a permission as the provided user.
    static void set_permission(std::shared_ptr<SyncUser>, Permission, PermissionChangeCallback, const ConfigMaker&);

    // Delete a permission as the provided user.
    static void delete_permission(std::shared_ptr<SyncUser>, Permission, PermissionChangeCallback, const ConfigMaker&);

private:
    static SharedRealm management_realm(std::shared_ptr<SyncUser>, const ConfigMaker&);
    static SharedRealm permission_realm(std::shared_ptr<SyncUser>, const ConfigMaker&);
};

struct PermissionChangeException : std::runtime_error {
    size_t code;

    PermissionChangeException(std::string message, size_t code)
    : std::runtime_error(std::move(message))
    , code(code)
    { }
};

}

#endif /* REALM_OS_SYNC_PERMISSION_HPP */
