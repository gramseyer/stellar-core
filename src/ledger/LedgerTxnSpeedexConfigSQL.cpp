#include "ledger/LedgerTxnImpl.h"
#include "ledger/NonSociRelatedException.h"
#include "util/types.h"

namespace stellar
{

static void
throwIfNotSpeedexConfig(LedgerEntryType type)
{
    if (type != SPEEDEX_CONFIG)
    {
        throw NonSociRelatedException("LedgerEntry is not a SPEEDEX_CONFIG");
    }
}

LedgerEntry getDefaultSpeedexConfig() {
    LedgerEntry entry;
    entry.data.type(SPEEDEX_CONFIG);
    //TODO mock out default config
    return entry;
}

static LedgerEntry currentSpeedexConfig = getDefaultSpeedexConfig();

std::shared_ptr<LedgerEntry const>
LedgerTxnRoot::Impl::loadSpeedexConfig(LedgerKey const& key) const
{
    throwIfNotSpeedexConfig(key.type());   
    return std::make_shared<LedgerEntry const>(currentSpeedexConfig);
}

class BulkLoadSpeedexConfigOperation
    : public DatabaseTypeSpecificOperation<std::vector<LedgerEntry>>
{
    Database& mDb;

  public:
    BulkLoadSpeedexConfigOperation(Database& db,
                                   UnorderedSet<LedgerKey> const& keys)
        : mDb(db)
    {
    }

    std::vector<LedgerEntry>
    doSqliteSpecificOperation(soci::sqlite3_session_backend* sq) override
    {
        return {currentSpeedexConfig};
    }

#ifdef USE_POSTGRES
    std::vector<LedgerEntry>
    doPostgresSpecificOperation(soci::postgresql_session_backend* pg) override
    {

        return {currentSpeedexConfig};
    }
#endif
};

UnorderedMap<LedgerKey, std::shared_ptr<LedgerEntry const>>
LedgerTxnRoot::Impl::bulkLoadSpeedexConfig(
    UnorderedSet<LedgerKey> const& keys) const
{
    if (!keys.empty())
    {
        BulkLoadSpeedexConfigOperation op(mDatabase, keys);
        return populateLoadedEntries(
            keys, mDatabase.doDatabaseTypeSpecificOperation(op));
    }
    else
    {
        return {};
    }
}

class BulkDeleteSpeedexConfigOperation
    : public DatabaseTypeSpecificOperation<void>
{
    Database& mDb;
    LedgerTxnConsistency mCons;

  public:
    BulkDeleteSpeedexConfigOperation(Database& db, LedgerTxnConsistency cons,
                                     std::vector<EntryIterator> const& entries)
        : mDb(db), mCons(cons)
    {
    }

    void
    doSociGenericOperation()
    {

    }

    void
    doSqliteSpecificOperation(soci::sqlite3_session_backend* sq) override
    {
    }

#ifdef USE_POSTGRES
    void
    doPostgresSpecificOperation(soci::postgresql_session_backend* pg) override
    {
    }
#endif
};

void
LedgerTxnRoot::Impl::bulkDeleteSpeedexConfig (
    std::vector<EntryIterator> const& entries, LedgerTxnConsistency cons)
{
    BulkDeleteSpeedexConfigOperation op(mDatabase, cons, entries);
    mDatabase.doDatabaseTypeSpecificOperation(op);
    if (entries.size() > 0) {
        currentSpeedexConfig = getDefaultSpeedexConfig();
    }
}

class BulkUpsertSpeedexConfigOperation
    : public DatabaseTypeSpecificOperation<void>
{
    Database& mDb;

  public:
    BulkUpsertSpeedexConfigOperation(
        Database& Db, std::vector<EntryIterator> const& entryIter)
        : mDb(Db)
    {
    }

    void
    doSociGenericOperation()
    {

    }

    void
    doSqliteSpecificOperation(soci::sqlite3_session_backend* sq) override
    {
    }

#ifdef USE_POSTGRES
    void
    doPostgresSpecificOperation(soci::postgresql_session_backend* pg) override
    {

    }
#endif
};

void
LedgerTxnRoot::Impl::bulkUpsertSpeedexConfig(
    std::vector<EntryIterator> const& entries)
{
    BulkUpsertSpeedexConfigOperation op(mDatabase, entries);
    mDatabase.doDatabaseTypeSpecificOperation(op);
    for (auto const& entry : entries) {
        currentSpeedexConfig = entry.entry().ledgerEntry();
    }
}

void
LedgerTxnRoot::Impl::dropSpeedexConfigs()
{
    throwIfChild();
    //TODO do something, right now no-op
    //no op
    currentSpeedexConfig = getDefaultSpeedexConfig();
    clearAllCaches();
}

} /* namespace stellar */