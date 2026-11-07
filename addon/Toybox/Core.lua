-- Talks to mod-toybox and holds what it said. The server decides everything: this only
-- displays its state and asks it to act, so a modified addon can't grant anything
local _, ns = ...

local PREFIX = "TOYBOX"

ns.toys = {}       -- sorted list of { id, name }
ns.owned = {}      -- [itemID] = true
ns.cooldowns = {}  -- [itemID] = { start, duration, spell }, on the GetTime() clock
ns.connected = false

local byId = {}
local listeners = {}

function ns.OnChange(callback)
    listeners[#listeners + 1] = callback
end

local function changed()
    for _, callback in ipairs(listeners) do
        callback()
    end
end

local function send(body)
    SendAddonMessage(PREFIX, body, "WHISPER", UnitName("player"))
end

function ns.Use(itemID)
    send("use:" .. itemID)
end

function ns.Name(itemID)
    local toy = byId[itemID]
    return toy and toy.name
end

function ns.Find(text)
    local id = tonumber(text)
    if id then
        return byId[id] and id
    end
    text = text:lower()
    for _, toy in ipairs(ns.toys) do
        if toy.name:lower() == text then
            return toy.id
        end
    end
end

function ns.CooldownRemaining(itemID)
    local cooldown = ns.cooldowns[itemID]
    if not cooldown then
        return 0
    end
    local remaining = cooldown.start + cooldown.duration - GetTime()
    if remaining <= 0 then
        ns.cooldowns[itemID] = nil
        return 0
    end
    return remaining
end

-- Sets a Cooldown frame to match the toy, which is what ElvUI and OmniCC hook
function ns.ApplyCooldown(frame, itemID)
    local cooldown = ns.cooldowns[itemID]
    if cooldown and ns.CooldownRemaining(itemID) > 0 then
        CooldownFrame_SetTimer(frame, cooldown.start, cooldown.duration, 1)
    else
        CooldownFrame_SetTimer(frame, 0, 0, 0)
    end
end

function ns.SetTooltip(owner, itemID)
    GameTooltip:SetOwner(owner, "ANCHOR_RIGHT")
    GameTooltip:SetHyperlink("item:" .. itemID)
    GameTooltip:Show()
end

local function addCatalog(entries)
    for id, name in entries:gmatch("(%d+)=([^;]+)") do
        id = tonumber(id)
        local toy = { id = id, name = name }
        byId[id] = toy
        ns.toys[#ns.toys + 1] = toy
    end
end

local handlers = {
    reset = function()
        wipe(ns.toys)
        wipe(ns.owned)
        wipe(byId)
    end,
    cat = addCatalog,
    own = function(ids)
        for id in ids:gmatch("%d+") do
            ns.owned[tonumber(id)] = true
        end
    end,
    done = function()
        table.sort(ns.toys, function(a, b)
            return a.name < b.name
        end)
        ns.connected = true
        changed()
    end,
    cd = function(args)
        local id, delay, spell = args:match("^(%d+):(%d+):(%d+)$")
        ns.cooldowns[tonumber(id)] = { start = GetTime(), duration = tonumber(delay) / 1000, spell = tonumber(spell) }
        changed()
    end,
    confirm = function(id)
        local name = ns.Name(tonumber(id)) or "This toy"
        StaticPopup_Show("TOYBOX_CONFIRM", name)
    end,
}

StaticPopupDialogs["TOYBOX_CONFIRM"] = {
    text = "%s is tradable. Add it to your toybox? This destroys the item.",
    button1 = YES,
    button2 = NO,
    OnAccept = function()
        send("confirm")
    end,
    timeout = 30,
    whileDead = true,
    hideOnEscape = true,
}

-- Nothing tells the addon when a cooldown is cleared early, such as by a GM. The client's
-- own cooldowns do clear, though only one of these reports a toy, depending on whether it
-- was used this session or restored at login, so a toy is ready once both are empty
local function dropClearedCooldowns()
    local dropped = false
    for itemID, cooldown in pairs(ns.cooldowns) do
        local _, spellDuration = GetSpellCooldown(cooldown.spell)
        local _, itemDuration = GetItemCooldown(itemID)
        if spellDuration == 0 and itemDuration == 0 then
            ns.cooldowns[itemID] = nil
            dropped = true
        end
    end
    if dropped then
        changed()
    end
end

local events = CreateFrame("Frame")
events:RegisterEvent("PLAYER_ENTERING_WORLD")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("SPELL_UPDATE_COOLDOWN")
events:SetScript("OnEvent", function(self, event, prefix, message)
    if event == "SPELL_UPDATE_COOLDOWN" then
        dropClearedCooldowns()
    elseif event == "PLAYER_ENTERING_WORLD" then
        self:UnregisterEvent(event)
        send("hello")
    elseif prefix == PREFIX then
        local tag, args = message:match("^(%a+):(.*)$")
        if handlers[tag] then
            handlers[tag](args)
        end
    end
end)

SLASH_TOY1 = "/toy"
SlashCmdList.TOY = function(text)
    local id = ns.Find(text:trim())
    if id and ns.owned[id] then
        ns.Use(id)
    else
        print("|cff33ff99Toybox:|r you don't have a toy called " .. text)
    end
end
