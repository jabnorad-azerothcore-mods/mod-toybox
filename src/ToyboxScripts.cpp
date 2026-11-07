#include "AccountMgr.h"
#include "AccountScript.h"
#include "AllItemScript.h"
#include "AllSpellScript.h"
#include "Chat.h"
#include "ChatCommand.h"
#include "CommandScript.h"
#include "Player.h"
#include "PlayerScript.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "Toybox.h"
#include "WorldScript.h"

using namespace Acore::ChatCommands;

class ToyboxWorldScript : public WorldScript
{
public:
    ToyboxWorldScript() : WorldScript("ToyboxWorldScript", { WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sToybox->LoadConfig();
    }

    void OnStartup() override
    {
        sToybox->LoadCatalog();
    }
};

class ToyboxAccountScript : public AccountScript
{
public:
    ToyboxAccountScript() : AccountScript("ToyboxAccountScript", { ACCOUNTHOOK_ON_BEFORE_ACCOUNT_DELETE }) { }

    void OnBeforeAccountDelete(uint32 accountId) override
    {
        sToybox->OnAccountDelete(accountId);
    }
};

class ToyboxPlayerScript : public PlayerScript
{
public:
    ToyboxPlayerScript() : PlayerScript("ToyboxPlayerScript", { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_LOGOUT, PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT }) { }

    void OnPlayerLogin(Player* player) override
    {
        sToybox->OnLogin(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        sToybox->OnLogout(player);
    }

    bool OnPlayerCanUseChat(Player* player, uint32 /*type*/, uint32 language, std::string& msg, Player* receiver) override
    {
        if (language != LANG_ADDON || receiver != player)
            return true;
        return !sToybox->HandleAddonMessage(player, msg);
    }
};

class ToyboxItemScript : public AllItemScript
{
public:
    ToyboxItemScript() : AllItemScript("ToyboxItemScript") { }

    bool CanItemUse(Player* player, Item* item, SpellCastTargets const& /*targets*/) override
    {
        return sToybox->HandleItemUse(player, item);
    }
};

class ToyboxSpellScript : public AllSpellScript
{
public:
    ToyboxSpellScript() : AllSpellScript("ToyboxSpellScript", { ALLSPELLHOOK_ON_CAST, ALLSPELLHOOK_ON_CAST_CANCEL }) { }

    void OnSpellCast(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        if (Player* player = caster ? caster->ToPlayer() : nullptr)
            sToybox->OnSpellCast(player, spellInfo->Id);
    }

    void OnSpellCastCancel(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo, bool /*bySelf*/) override
    {
        if (Player* player = caster ? caster->ToPlayer() : nullptr)
            sToybox->OnSpellCancelled(player, spellInfo->Id);
    }
};

class ToyboxCommandScript : public CommandScript
{
public:
    ToyboxCommandScript() : CommandScript("ToyboxCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable toyboxTable =
        {
            { "use",    HandleUse,    rbac::RBAC_PERM_COMMAND_COMMANDS,               Console::No },
            { "list",   HandleList,   rbac::RBAC_PERM_COMMAND_COMMANDS,               Console::No },
            { "link",   HandleLink,   rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_SECLEVEL, Console::Yes },
            { "unlink", HandleUnlink, rbac::RBAC_PERM_COMMAND_ACCOUNT_SET_SECLEVEL, Console::Yes }
        };

        static ChatCommandTable commandTable =
        {
            { "toybox", toyboxTable }
        };
        return commandTable;
    }

    static bool HandleUse(ChatHandler* handler, uint32 itemId)
    {
        sToybox->UseToy(handler->GetPlayer(), itemId);
        return true;
    }

    static bool HandleList(ChatHandler* handler)
    {
        handler->PSendSysMessage("Toybox: {} of {} toys collected.",
            sToybox->OwnedCount(handler->GetSession()->GetAccountId()), sToybox->CatalogSize());
        return true;
    }

    static bool HandleLink(ChatHandler* handler, std::string account, std::string sharedWith)
    {
        if (!sToybox->IsSharingEnabled())
        {
            handler->SendErrorMessage("Set Toybox.SharedAccounts.Enable = 1 to share toyboxes.");
            return false;
        }

        uint32 accountId = AccountMgr::GetId(account);
        uint32 sharedWithId = AccountMgr::GetId(sharedWith);
        if (!accountId || !sharedWithId)
        {
            handler->SendErrorMessage("Account not found.");
            return false;
        }

        sToybox->LinkAccounts(accountId, sharedWithId);
        handler->PSendSysMessage("{} now shares a toybox with {}.", account, sharedWith);
        return true;
    }

    static bool HandleUnlink(ChatHandler* handler, std::string account)
    {
        uint32 accountId = AccountMgr::GetId(account);
        if (!accountId)
        {
            handler->SendErrorMessage("Account not found.");
            return false;
        }

        sToybox->UnlinkAccount(accountId);
        handler->PSendSysMessage("{} has its own toybox again, starting empty.", account);
        return true;
    }
};

void AddToyboxScripts()
{
    new ToyboxWorldScript();
    new ToyboxAccountScript();
    new ToyboxPlayerScript();
    new ToyboxItemScript();
    new ToyboxSpellScript();
    new ToyboxCommandScript();
}
