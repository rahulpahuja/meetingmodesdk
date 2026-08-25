#include "meeting_sdk/storage/sqlite_meeting_repository.hpp"

#include <sqlite3.h>

#include "meeting_sdk/storage/meeting_serializer.hpp"

namespace meeting_sdk::storage {
namespace {

core::Error sqliteError(sqlite3* db, const std::string& code) {
    return core::Error{
        .category = core::ErrorCategory::Storage,
        .code = code,
        .message = db != nullptr ? sqlite3_errmsg(db) : "sqlite error",
    };
}

}  // namespace

core::Result<std::unique_ptr<SqliteMeetingRepository>> SqliteMeetingRepository::open(
    std::string dbPath, IEncryptor& encryptor, IKeyProvider& keyProvider) {
    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        core::Error error = sqliteError(db, "storage.open_failed");
        sqlite3_close(db);
        return error;
    }

    const char* createTable =
        "CREATE TABLE IF NOT EXISTS meetings (id TEXT PRIMARY KEY, ciphertext BLOB NOT NULL);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        core::Error error{
            .category = core::ErrorCategory::Storage,
            .code = "storage.schema_init_failed",
            .message = errMsg != nullptr ? errMsg : "unknown schema error",
        };
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return error;
    }

    return std::unique_ptr<SqliteMeetingRepository>(
        new SqliteMeetingRepository(db, encryptor, keyProvider));
}

SqliteMeetingRepository::SqliteMeetingRepository(sqlite3* db, IEncryptor& encryptor, IKeyProvider& keyProvider)
    : db_(db), encryptor_(encryptor), keyProvider_(keyProvider) {}

SqliteMeetingRepository::~SqliteMeetingRepository() {
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
}

core::Result<void> SqliteMeetingRepository::save(const core::Meeting& meeting) {
    auto key = keyProvider_.getOrCreateKey(meeting.id);
    if (!key) {
        return key.error();
    }

    const auto plaintext = MeetingSerializer::serialize(meeting);
    auto ciphertext = encryptor_.encrypt(plaintext, key.value());
    if (!ciphertext) {
        return ciphertext.error();
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO meetings (id, ciphertext) VALUES (?, ?) "
        "ON CONFLICT(id) DO UPDATE SET ciphertext = excluded.ciphertext;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sqliteError(db_, "storage.prepare_failed");
    }
    sqlite3_bind_text(stmt, 1, meeting.id.value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 2, ciphertext.value().data(), static_cast<int>(ciphertext.value().size()),
                       SQLITE_TRANSIENT);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        return sqliteError(db_, "storage.write_failed");
    }
    return {};
}

core::Result<core::Meeting> SqliteMeetingRepository::get(const core::MeetingId& id) {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT ciphertext FROM meetings WHERE id = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sqliteError(db_, "storage.prepare_failed");
    }
    sqlite3_bind_text(stmt, 1, id.value.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return core::Error{
            .category = core::ErrorCategory::Storage,
            .code = "storage.not_found",
            .message = "no meeting with id '" + id.value + "'",
        };
    }

    const auto* blob = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 0));
    const int blobLen = sqlite3_column_bytes(stmt, 0);
    std::vector<std::uint8_t> ciphertext(blob, blob + blobLen);
    sqlite3_finalize(stmt);

    auto key = keyProvider_.getOrCreateKey(id);
    if (!key) {
        return key.error();
    }

    auto plaintext = encryptor_.decrypt(ciphertext, key.value());
    if (!plaintext) {
        return plaintext.error();
    }

    return MeetingSerializer::deserialize(plaintext.value());
}

core::Result<void> SqliteMeetingRepository::remove(const core::MeetingId& id) {
    // Crypto-erase first: even if the row delete below fails, the ciphertext is already
    // unrecoverable once the key is gone.
    auto keyResult = keyProvider_.deleteKey(id);
    if (!keyResult) {
        return keyResult.error();
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "DELETE FROM meetings WHERE id = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sqliteError(db_, "storage.prepare_failed");
    }
    sqlite3_bind_text(stmt, 1, id.value.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        return sqliteError(db_, "storage.delete_failed");
    }
    return {};
}

core::Result<std::vector<core::MeetingId>> SqliteMeetingRepository::listAll() {
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM meetings ORDER BY id;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return sqliteError(db_, "storage.prepare_failed");
    }
    std::vector<core::MeetingId> ids;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        ids.push_back(core::MeetingId{text != nullptr ? text : ""});
    }
    sqlite3_finalize(stmt);
    return ids;
}

}  // namespace meeting_sdk::storage
