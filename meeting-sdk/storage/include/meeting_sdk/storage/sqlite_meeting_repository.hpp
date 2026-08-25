#pragma once

#include <memory>
#include <string>

#include "meeting_sdk/core/interfaces.hpp"
#include "meeting_sdk/storage/encryptor.hpp"
#include "meeting_sdk/storage/key_provider.hpp"

struct sqlite3;  // keep sqlite3.h out of the public header

namespace meeting_sdk::storage {

// core::IMeetingRepository backed by SQLite, with every stored Meeting encrypted at rest via
// IEncryptor under a per-meeting key from IKeyProvider. Only ciphertext ever touches disk —
// serialization happens before encryption, decryption before deserialization — so a
// compromised disk image yields no readable meeting content without the key (see
// docs/architecture/03-threat-model-and-security.md §5). remove() deletes the key before the
// row: the crypto-erase guarantee holds even if the row deletion itself later fails.
class SqliteMeetingRepository final : public core::IMeetingRepository {
public:
    // dbPath: a file path, or ":memory:" for an ephemeral in-process database (used by tests).
    // encryptor/keyProvider must outlive this repository.
    static core::Result<std::unique_ptr<SqliteMeetingRepository>> open(std::string dbPath,
                                                                         IEncryptor& encryptor,
                                                                         IKeyProvider& keyProvider);

    ~SqliteMeetingRepository() override;
    SqliteMeetingRepository(const SqliteMeetingRepository&) = delete;
    SqliteMeetingRepository& operator=(const SqliteMeetingRepository&) = delete;

    core::Result<void> save(const core::Meeting& meeting) override;
    core::Result<core::Meeting> get(const core::MeetingId& id) override;
    core::Result<void> remove(const core::MeetingId& id) override;
    core::Result<std::vector<core::MeetingId>> listAll() override;

private:
    SqliteMeetingRepository(sqlite3* db, IEncryptor& encryptor, IKeyProvider& keyProvider);

    sqlite3* db_;
    IEncryptor& encryptor_;
    IKeyProvider& keyProvider_;
};

}  // namespace meeting_sdk::storage
