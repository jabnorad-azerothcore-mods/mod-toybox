-- A small bar of chosen toys. A normal action bar can't hold a toy with no item, so this
-- is what gives them a place on screen with a live cooldown. Right-click a toy in the
-- window to add or remove it. /toybox bar shows or hides it, /toybox move unlocks it
local _, ns = ...

local MAX_BUTTONS = 12
local SIZE, GAP = 36, 4

local bar = CreateFrame("Frame", "ToyboxBar", UIParent)
bar:SetSize(SIZE, SIZE)
bar:SetPoint("CENTER", 0, -200)
bar:SetMovable(true)
bar:SetClampedToScreen(true)
bar:RegisterForDrag("LeftButton")
bar:SetScript("OnDragStart", bar.StartMoving)
bar:SetScript("OnDragStop", function(self)
    self:StopMovingOrSizing()
    local point, _, relative, x, y = self:GetPoint()
    ToyboxCharDB.position = { point, relative, x, y }
end)
ns.bar = bar

bar.handle = bar:CreateTexture(nil, "OVERLAY")
bar.handle:SetAllPoints()
bar.handle:SetTexture(0, 0.6, 1, 0.3)
bar.handle:Hide()

local buttons = {}

local function createButton(index)
    local button = CreateFrame("Button", "ToyboxBarButton" .. index, bar)
    button:SetSize(SIZE, SIZE)
    button:SetPoint("LEFT", (index - 1) * (SIZE + GAP), 0)
    button:RegisterForClicks("LeftButtonUp", "RightButtonUp")
    button.icon = button:CreateTexture(nil, "ARTWORK")
    button.icon:SetAllPoints()
    button.cooldown = CreateFrame("Cooldown", "ToyboxBarButton" .. index .. "Cooldown", button, "CooldownFrameTemplate")
    button.cooldown:SetAllPoints()
    button:SetHighlightTexture("Interface\\Buttons\\ButtonHilight-Square", "ADD")
    button:SetScript("OnEnter", function(self)
        ns.SetTooltip(self, self.id)
    end)
    button:SetScript("OnLeave", GameTooltip_Hide)
    button:SetScript("OnClick", function(self, mouse)
        if mouse == "RightButton" then
            ns.ToggleOnBar(self.id)
        else
            ns.Use(self.id)
        end
    end)
    buttons[index] = button
    if ns.SkinBarButton then
        ns.SkinBarButton(button)
    end
    return button
end

local function refresh()
    if not ToyboxCharDB then
        return
    end

    local shown = 0
    for _, id in ipairs(ToyboxCharDB.bar) do
        if ns.owned[id] and shown < MAX_BUTTONS then
            shown = shown + 1
            local button = buttons[shown] or createButton(shown)
            button.id = id
            button.icon:SetTexture(GetItemIcon(id))
            ns.ApplyCooldown(button.cooldown, id)
            button:Show()
        end
    end
    for index = shown + 1, #buttons do
        buttons[index]:Hide()
    end
    bar:SetWidth(math.max(1, shown) * (SIZE + GAP) - GAP)
    if bar:IsMouseEnabled() or (shown > 0 and not ToyboxCharDB.hidden) then
        bar:Show()
    else
        bar:Hide()
    end
end

function ns.ToggleOnBar(itemID)
    local list = ToyboxCharDB.bar
    for index, id in ipairs(list) do
        if id == itemID then
            tremove(list, index)
            refresh()
            return
        end
    end
    if #list < MAX_BUTTONS then
        list[#list + 1] = itemID
        refresh()
    end
end

function ns.ToggleBar()
    ToyboxCharDB.hidden = not ToyboxCharDB.hidden
    refresh()
end

function ns.ToggleBarLock()
    local unlocked = not bar:IsMouseEnabled()
    bar:EnableMouse(unlocked)
    if unlocked then
        bar.handle:Show()
    else
        bar.handle:Hide()
    end
    refresh()
end

ns.OnChange(refresh)

local events = CreateFrame("Frame")
events:RegisterEvent("PLAYER_LOGIN")
events:SetScript("OnEvent", function()
    ToyboxCharDB = ToyboxCharDB or {}
    ToyboxCharDB.bar = ToyboxCharDB.bar or {}
    local position = ToyboxCharDB.position
    if position then
        bar:ClearAllPoints()
        bar:SetPoint(position[1], UIParent, position[2], position[3], position[4])
    end
    refresh()
end)
