// Archivo: src/PortalCore/Auth/AccountManager.h
#pragma once

#include "PortalCore/Common.h"
#include "PortalCore/Auth/PSNAuth.h"
#include <string>
#include <optional>
#include <vector>

namespace portal::auth {

struct PSNAccount {
    uint64_t account_id = 0;
    std::string account_id_b64;
    std::string npsso;
    PSNTokens tokens;
    PSNProfile profile;
};

class AccountManager {
public:
    Result<void> add_account(const std::string& npsso);
    Result<void> add_account_from_code(const std::string& auth_code);
    void remove_account(uint64_t account_id);
    std::optional<PSNAccount> get_active_account() const;
    Result<std::string> get_access_token();

    Result<void> save_to_disk();
    Result<void> load_from_disk();

private:
    std::vector<PSNAccount> accounts;
    std::optional<uint64_t> active_account_id;
};

} // namespace portal::auth
