#include "Toybox.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "Tokenize.h"
#include "Unit.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "WorldSessionMgr.h"
#include <vector>

namespace
{
constexpr char const* AddonPrefix = "TOYBOX";
constexpr uint32 ProtocolVersion = 1;
constexpr uint32 ConfirmSeconds = 30;
// An addon message holds 255 characters, including the prefix and its separator
constexpr std::size_t MaxMessageBody = 240;

std::set<uint32> ParseItemList(std::string const& list)
{
    std::set<uint32> items;
    for (std::string_view token : Acore::Tokenize(list, ',', false))
        if (Optional<uint32> id = Acore::StringTo<uint32>(token))
            items.insert(*id);
    return items;
}

// A toy has to be used from the bags with no limit on uses. Right-clicking an equippable item
// equips it instead of using it, and a charged item would be used up
bool CanBeToy(ItemTemplate const* proto)
{
    if (proto->InventoryType != INVTYPE_NON_EQUIP)
        return false;
    for (auto const& spell : proto->Spells)
        if (spell.SpellId > 0 && spell.SpellCharges != 0)
            return false;
    return true;
}

uint32 OnUseSpell(ItemTemplate const* proto)
{
    for (auto const& spell : proto->Spells)
        if (spell.SpellId > 0 && spell.SpellTrigger == ITEM_SPELLTRIGGER_ON_USE)
            return uint32(spell.SpellId);
    return 0;
}

void SendAddon(Player* player, std::string const& body)
{
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player,
        std::string(AddonPrefix) + "\t" + body);
    player->SendDirectMessage(&data);
}

// Item lists can be longer than one message, so they're sent as tag:a,b,c in pieces
template <typename Container, typename Format>
void SendInPieces(Player* player, std::string const& tag, Container const& items, Format format)
{
    std::string line;
    for (auto const& item : items)
    {
        std::string entry = format(item);
        if (entry.empty())
            continue;
        if (!line.empty() && tag.size() + 1 + line.size() + 1 + entry.size() > MaxMessageBody)
        {
            SendAddon(player, tag + ":" + line);
            line.clear();
        }
        if (!line.empty())
            line += ";";
        line += entry;
    }
    if (!line.empty())
        SendAddon(player, tag + ":" + line);
}

void Notify(Player* player, std::string const& text)
{
    ChatHandler(player->GetSession()).SendSysMessage("|cff33ff99Toybox:|r " + text);
}

void SendCooldown(Player* player, uint32 itemId, uint32 spellId)
{
    if (uint32 delay = player->GetSpellCooldownDelay(spellId))
        SendAddon(player, "cd:" + std::to_string(itemId) + ":" + std::to_string(delay) + ":" + std::to_string(spellId));
}
}

Toybox* Toybox::instance()
{
    static Toybox instance;
    return &instance;
}

void Toybox::LoadConfig()
{
    _enabled = sConfigMgr->GetOption<bool>("Toybox.Enable", false);
    _confirmTradable = sConfigMgr->GetOption<bool>("Toybox.ConfirmTradable", true);
    _sharingEnabled = sConfigMgr->GetOption<bool>("Toybox.SharedAccounts.Enable", false);
    _extraItems = ParseItemList(sConfigMgr->GetOption<std::string>("Toybox.ExtraItems", ""));
    _excludedItems = ParseItemList(sConfigMgr->GetOption<std::string>("Toybox.ExcludedItems", ""));
}

void Toybox::LoadCatalog()
{
    _catalog.clear();
    if (!_enabled)
        return;

    auto add = [this](uint32 itemId, uint8 target)
    {
        if (_excludedItems.count(itemId))
            return;

        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        uint32 spellId = proto ? OnUseSpell(proto) : 0;
        if (!spellId || !sSpellMgr->GetSpellInfo(spellId))
        {
            LOG_ERROR("module", "mod-toybox: item {} has no usable on-use spell, skipping it", itemId);
            return;
        }

        if (!CanBeToy(proto))
        {
            LOG_ERROR("module", "mod-toybox: item {} is equippable or has limited charges, skipping it", itemId);
            return;
        }

        _catalog[itemId] = { spellId, target <= TOY_TARGET_FEET ? ToyTarget(target) : TOY_TARGET_SELF, proto->Name1 };
    };

    if (QueryResult result = WorldDatabase.Query("SELECT item_id, target_type FROM mod_toybox_item"))
    {
        do
        {
            Field* fields = result->Fetch();
            add(fields[0].Get<uint32>(), fields[1].Get<uint8>());
        } while (result->NextRow());
    }

    for (uint32 itemId : _extraItems)
        if (!_catalog.count(itemId))
            add(itemId, TOY_TARGET_SELF);

    LOG_INFO("module", "mod-toybox: loaded {} toys", _catalog.size());
}

uint32 Toybox::ResolveToybox(uint32 accountId) const
{
    if (QueryResult result = LoginDatabase.Query("SELECT toybox_id FROM mod_toybox_account WHERE account_id = {}", accountId))
        return result->Fetch()[0].Get<uint32>();
    return accountId;
}

void Toybox::LoadCollection(uint32 accountId)
{
    uint32 toyboxId = ResolveToybox(accountId);
    std::set<uint32> items;
    if (QueryResult result = LoginDatabase.Query("SELECT item_id FROM mod_toybox_collection WHERE toybox_id = {}", toyboxId))
    {
        do
        {
            items.insert(result->Fetch()[0].Get<uint32>());
        } while (result->NextRow());
    }

    std::lock_guard<std::recursive_mutex> guard(_lock);
    _accountToybox[accountId] = toyboxId;
    _collections[toyboxId] = std::move(items);
}

void Toybox::OnLogin(Player* player)
{
    if (_enabled)
        LoadCollection(player->GetSession()->GetAccountId());
}

void Toybox::OnLogout(Player* player)
{
    uint32 accountId = player->GetSession()->GetAccountId();
    std::lock_guard<std::recursive_mutex> guard(_lock);
    _addonAccounts.erase(accountId);
    _pending.erase(accountId);
    _casts.erase(player->GetGUID().GetCounter());
}

// A toybox outlives the account it's named after while another account still uses it, so
// its toys only go with the last account
void Toybox::OnAccountDelete(uint32 accountId)
{
    uint32 toyboxId = ResolveToybox(accountId);
    LoginDatabase.DirectExecute("DELETE FROM mod_toybox_account WHERE account_id = {}", accountId);

    bool inUse = LoginDatabase.Query("SELECT 1 FROM mod_toybox_account WHERE toybox_id = {} LIMIT 1", toyboxId) != nullptr;
    if (!inUse && toyboxId != accountId)
        inUse = LoginDatabase.Query("SELECT 1 FROM account WHERE id = {}", toyboxId) != nullptr;
    if (!inUse)
        LoginDatabase.DirectExecute("DELETE FROM mod_toybox_collection WHERE toybox_id = {}", toyboxId);

    std::lock_guard<std::recursive_mutex> guard(_lock);
    _accountToybox.erase(accountId);
    if (!inUse)
        _collections.erase(toyboxId);
}

bool Toybox::Owns(uint32 accountId, uint32 itemId)
{
    std::lock_guard<std::recursive_mutex> guard(_lock);
    auto toybox = _accountToybox.find(accountId);
    if (toybox == _accountToybox.end())
        return false;
    auto collection = _collections.find(toybox->second);
    return collection != _collections.end() && collection->second.count(itemId);
}

std::size_t Toybox::OwnedCount(uint32 accountId)
{
    std::lock_guard<std::recursive_mutex> guard(_lock);
    auto toybox = _accountToybox.find(accountId);
    if (toybox == _accountToybox.end())
        return 0;
    return _collections[toybox->second].size();
}

bool Toybox::HandleItemUse(Player* player, Item* item)
{
    if (!_enabled)
        return false;

    auto toy = _catalog.find(item->GetEntry());
    if (toy == _catalog.end())
        return false;

    // The client marks an item busy when it's used; a failed cast clears that
    if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(toy->second.SpellId))
        Spell::SendCastResult(player, spellInfo, 0, SPELL_FAILED_DONT_REPORT);

    if (_confirmTradable && !item->IsSoulBound())
    {
        uint32 accountId = player->GetSession()->GetAccountId();
        uint32 now = uint32(GameTime::GetGameTime().count());

        std::lock_guard<std::recursive_mutex> guard(_lock);
        auto pending = _pending.find(accountId);
        bool confirmed = pending != _pending.end() && pending->second.ItemGuid == item->GetGUID() && pending->second.ExpiresAt >= now;
        if (!confirmed)
        {
            _pending[accountId] = { item->GetGUID(), now + ConfirmSeconds };
            Notify(player, toy->second.Name + " is tradable, and adding it to your toybox destroys the item. Use it again within "
                + std::to_string(ConfirmSeconds) + " seconds to confirm.");
            SendAddon(player, "confirm:" + std::to_string(item->GetEntry()));
            return true;
        }
        _pending.erase(pending);
    }

    Consume(player, item);
    return true;
}

void Toybox::ConfirmPending(Player* player)
{
    uint32 accountId = player->GetSession()->GetAccountId();
    uint32 now = uint32(GameTime::GetGameTime().count());
    ObjectGuid itemGuid;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        auto pending = _pending.find(accountId);
        if (pending == _pending.end())
            return;
        if (pending->second.ExpiresAt >= now)
            itemGuid = pending->second.ItemGuid;
        _pending.erase(pending);
    }

    if (Item* item = player->GetItemByGuid(itemGuid))
        Consume(player, item);
}

void Toybox::Consume(Player* player, Item* item)
{
    uint32 itemId = item->GetEntry();
    uint32 accountId = player->GetSession()->GetAccountId();
    std::string const& name = _catalog.at(itemId).Name;

    if (Owns(accountId, itemId))
        Notify(player, name + " is already in your toybox.");
    else
    {
        AddToCollection(player, item);
        Notify(player, name + " added to your toybox.");
    }

    uint32 count = 1;
    player->DestroyItemCount(item, count, true);
}

void Toybox::AddToCollection(Player* player, Item* item)
{
    uint32 accountId = player->GetSession()->GetAccountId();
    uint32 itemId = item->GetEntry();
    uint32 toyboxId;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        toyboxId = _accountToybox[accountId];
        _collections[toyboxId].insert(itemId);
    }

    // IGNORE: another account sharing this toybox can add the same toy at the same moment
    LoginDatabase.Execute("INSERT IGNORE INTO mod_toybox_collection (toybox_id, item_id) VALUES ({}, {})", toyboxId, itemId);
    SendOwnedToSharers(toyboxId);
}

void Toybox::UseToy(Player* player, uint32 itemId)
{
    if (!_enabled)
        return;

    auto toy = _catalog.find(itemId);
    if (toy == _catalog.end() || !Owns(player->GetSession()->GetAccountId(), itemId))
    {
        Notify(player, "that toy isn't in your toybox.");
        return;
    }

    ToyDefinition const& definition = toy->second;
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(definition.SpellId);
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!spellInfo || !proto)
        return;

    if (player->HasSpellCooldown(definition.SpellId))
    {
        SendCooldown(player, itemId, definition.SpellId);
        Notify(player, definition.Name + " isn't ready yet.");
        return;
    }

    // The same class, race and level limits the real item has
    InventoryResult usable = player->CanUseItem(proto);
    if (usable != EQUIP_ERR_OK)
    {
        player->SendEquipError(usable, nullptr, nullptr, itemId);
        return;
    }

    Unit* target = player;
    if (definition.Target == TOY_TARGET_SELECTED)
    {
        target = player->GetSelectedUnit();
        if (!target)
        {
            Notify(player, definition.Name + " needs a target.");
            return;
        }
    }

    // An instant cast finishes inside CastSpell, so this has to be set first
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        _casts[player->GetGUID().GetCounter()] = { itemId, definition.SpellId };
    }

    // Failures such as a missing reagent reach the client as normal cast errors
    SpellCastResult result;
    if (definition.Target == TOY_TARGET_FEET)
    {
        SpellCastTargets targets;
        targets.SetDst(*player);
        result = player->CastSpell(targets, spellInfo, nullptr, TRIGGERED_NONE);
    }
    else
        result = player->CastSpell(target, spellInfo, TRIGGERED_NONE);

    if (result != SPELL_CAST_OK)
        OnSpellCancelled(player, definition.SpellId);
}

// The cooldown has to wait for this. A spell with a cast time is checked again when the cast
// ends, and failing that check is all a cooldown set at the start would do
void Toybox::OnSpellCast(Player* player, uint32 spellId)
{
    ToyCast cast;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        auto found = _casts.find(player->GetGUID().GetCounter());
        if (found == _casts.end() || found->second.SpellId != spellId)
            return;
        cast = found->second;
        _casts.erase(found);
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return;

    // The cooldown is on the item, so a cast with no item doesn't start it
    player->AddSpellAndCategoryCooldowns(spellInfo, cast.ItemId);
    if (uint32 delay = player->GetSpellCooldownDelay(spellId))
    {
        WorldPacket data;
        player->BuildCooldownPacket(data, SPELL_COOLDOWN_FLAG_NONE, spellId, delay);
        player->SendDirectMessage(&data);
    }
    SendCooldown(player, cast.ItemId, spellId);
}

void Toybox::OnSpellCancelled(Player* player, uint32 spellId)
{
    std::lock_guard<std::recursive_mutex> guard(_lock);
    auto found = _casts.find(player->GetGUID().GetCounter());
    if (found != _casts.end() && found->second.SpellId == spellId)
        _casts.erase(found);
}

void Toybox::SendCollection(Player* player)
{
    SendAddon(player, "reset:" + std::to_string(ProtocolVersion));
    SendInPieces(player, "cat", _catalog, [](auto const& toy)
    {
        return std::to_string(toy.first) + "=" + toy.second.Name;
    });

    uint32 accountId = player->GetSession()->GetAccountId();
    std::set<uint32> owned;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        owned = _collections[_accountToybox[accountId]];
    }

    SendInPieces(player, "own", owned, [this](uint32 itemId)
    {
        return _catalog.count(itemId) ? std::to_string(itemId) : std::string();
    });
    SendAddon(player, "done:" + std::to_string(owned.size()));

    for (uint32 itemId : owned)
    {
        auto toy = _catalog.find(itemId);
        if (toy != _catalog.end())
            SendCooldown(player, itemId, toy->second.SpellId);
    }
}

void Toybox::SendOwnedToSharers(uint32 toyboxId)
{
    std::vector<uint32> accounts;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        for (uint32 accountId : _addonAccounts)
        {
            auto toybox = _accountToybox.find(accountId);
            if (toybox != _accountToybox.end() && toybox->second == toyboxId)
                accounts.push_back(accountId);
        }
    }

    for (uint32 accountId : accounts)
        if (WorldSession* session = sWorldSessionMgr->FindSession(accountId))
            if (Player* player = session->GetPlayer())
                SendCollection(player);
}

bool Toybox::HandleAddonMessage(Player* player, std::string const& message)
{
    if (!_enabled)
        return false;

    std::string const start = std::string(AddonPrefix) + "\t";
    if (message.rfind(start, 0) != 0)
        return false;

    std::string body = message.substr(start.size());
    if (body == "hello")
    {
        {
            std::lock_guard<std::recursive_mutex> guard(_lock);
            _addonAccounts.insert(player->GetSession()->GetAccountId());
        }
        SendCollection(player);
    }
    else if (body == "confirm")
        ConfirmPending(player);
    else if (body.rfind("use:", 0) == 0)
    {
        if (Optional<uint32> itemId = Acore::StringTo<uint32>(std::string_view(body).substr(4)))
            UseToy(player, *itemId);
    }

    return true;
}

void Toybox::LinkAccounts(uint32 accountId, uint32 sharedWithAccountId)
{
    uint32 oldToybox = ResolveToybox(accountId);
    uint32 newToybox = ResolveToybox(sharedWithAccountId);
    if (oldToybox == newToybox)
        return;

    // Move every account on the old toybox, so nothing is left pointing at it
    LoginDatabaseTransaction trans = LoginDatabase.BeginTransaction();
    trans->Append("INSERT IGNORE INTO mod_toybox_collection (toybox_id, item_id, added_at) SELECT {}, item_id, added_at FROM mod_toybox_collection WHERE toybox_id = {}", newToybox, oldToybox);
    trans->Append("DELETE FROM mod_toybox_collection WHERE toybox_id = {}", oldToybox);
    trans->Append("UPDATE mod_toybox_account SET toybox_id = {} WHERE toybox_id = {}", newToybox, oldToybox);
    trans->Append("REPLACE INTO mod_toybox_account (account_id, toybox_id) VALUES ({}, {})", accountId, newToybox);
    LoginDatabase.DirectCommitTransaction(trans);

    std::vector<uint32> online;
    {
        std::lock_guard<std::recursive_mutex> guard(_lock);
        for (auto const& entry : _accountToybox)
            online.push_back(entry.first);
    }
    for (uint32 onlineAccount : online)
        LoadCollection(onlineAccount);
    LoadCollection(accountId);
    SendOwnedToSharers(newToybox);
}

void Toybox::UnlinkAccount(uint32 accountId)
{
    LoginDatabase.DirectExecute("DELETE FROM mod_toybox_account WHERE account_id = {}", accountId);
    LoadCollection(accountId);
    SendOwnedToSharers(ResolveToybox(accountId));
}
