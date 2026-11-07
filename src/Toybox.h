#ifndef MOD_TOYBOX_H
#define MOD_TOYBOX_H

#include "Define.h"
#include "ObjectGuid.h"
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Item;
class Player;

enum ToyTarget : uint8
{
    TOY_TARGET_SELF     = 0,
    TOY_TARGET_SELECTED = 1,
    TOY_TARGET_FEET     = 2
};

struct ToyDefinition
{
    uint32 SpellId = 0;
    ToyTarget Target = TOY_TARGET_SELF;
    std::string Name;
};

class Toybox
{
public:
    static Toybox* instance();

    void LoadConfig();
    void LoadCatalog();

    [[nodiscard]] bool IsEnabled() const { return _enabled; }
    [[nodiscard]] bool IsSharingEnabled() const { return _sharingEnabled; }
    [[nodiscard]] std::size_t CatalogSize() const { return _catalog.size(); }
    [[nodiscard]] std::size_t OwnedCount(uint32 accountId);

    void OnLogin(Player* player);
    void OnLogout(Player* player);
    void OnAccountDelete(uint32 accountId);

    // Returns true when the item is a toy, so its own spell must not be cast
    bool HandleItemUse(Player* player, Item* item);
    void ConfirmPending(Player* player);
    void UseToy(Player* player, uint32 itemId);
    void OnSpellCast(Player* player, uint32 spellId);
    void OnSpellCancelled(Player* player, uint32 spellId);

    // Returns false on a bad message, which then falls through as ordinary addon chat
    bool HandleAddonMessage(Player* player, std::string const& body);

    void LinkAccounts(uint32 accountId, uint32 sharedWithAccountId);
    void UnlinkAccount(uint32 accountId);

private:
    struct ToyCast
    {
        uint32 ItemId = 0;
        uint32 SpellId = 0;
    };

    struct PendingItem
    {
        ObjectGuid ItemGuid;
        uint32 ExpiresAt = 0;
    };

    [[nodiscard]] uint32 ResolveToybox(uint32 accountId) const;
    void LoadCollection(uint32 accountId);
    void AddToCollection(Player* player, Item* item);
    void Consume(Player* player, Item* item);
    void SendCollection(Player* player);
    void SendOwnedToSharers(uint32 toyboxId);
    [[nodiscard]] bool Owns(uint32 accountId, uint32 itemId);

    bool _enabled = false;
    bool _confirmTradable = true;
    bool _sharingEnabled = false;
    std::set<uint32> _extraItems;
    std::set<uint32> _excludedItems;

    // Filled at startup and read-only after
    std::map<uint32, ToyDefinition> _catalog;

    // Packets arrive on map threads, so everything below is under _lock
    std::recursive_mutex _lock;
    std::unordered_map<uint32, uint32> _accountToybox;
    std::unordered_map<uint32, std::set<uint32>> _collections;
    std::unordered_set<uint32> _addonAccounts;
    std::unordered_map<uint32, PendingItem> _pending;
    std::unordered_map<ObjectGuid::LowType, ToyCast> _casts;
};

#define sToybox Toybox::instance()

#endif
