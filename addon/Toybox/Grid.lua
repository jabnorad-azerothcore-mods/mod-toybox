-- The collection window: a page of every toy, with missing ones grayed out
local _, ns = ...

local COLUMNS, ROWS = 4, 5
local PAGE_SIZE = COLUMNS * ROWS
local CELL_WIDTH, CELL_HEIGHT = 150, 46

BINDING_HEADER_TOYBOX = "Toybox"
BINDING_NAME_TOYBOX_TOGGLE = "Open Toybox"

local page, search, hideMissing = 1, "", false
local cells = {}

local frame = CreateFrame("Frame", "ToyboxFrame", UIParent)
frame:SetSize(COLUMNS * CELL_WIDTH + 32, ROWS * CELL_HEIGHT + 120)
frame:SetPoint("CENTER")
frame:SetFrameStrata("HIGH")
frame:SetToplevel(true)
frame:SetMovable(true)
frame:EnableMouse(true)
frame:RegisterForDrag("LeftButton")
frame:SetScript("OnDragStart", frame.StartMoving)
frame:SetScript("OnDragStop", frame.StopMovingOrSizing)
frame:SetBackdrop({
    bgFile = "Interface\\DialogFrame\\UI-DialogBox-Background",
    edgeFile = "Interface\\DialogFrame\\UI-DialogBox-Border",
    tile = true, tileSize = 32, edgeSize = 32,
    insets = { left = 11, right = 12, top = 12, bottom = 11 },
})
frame:Hide()
tinsert(UISpecialFrames, "ToyboxFrame")
ns.frame = frame

frame.title = frame:CreateFontString(nil, "ARTWORK", "GameFontNormalLarge")
frame.title:SetPoint("TOP", 0, -18)

frame.close = CreateFrame("Button", "ToyboxFrameClose", frame, "UIPanelCloseButton")
frame.close:SetPoint("TOPRIGHT", -6, -6)

frame.search = CreateFrame("EditBox", "ToyboxFrameSearch", frame, "InputBoxTemplate")
frame.search:SetSize(170, 20)
frame.search:SetPoint("TOPLEFT", 24, -44)
frame.search:SetAutoFocus(false)

frame.missing = CreateFrame("CheckButton", "ToyboxFrameMissing", frame, "UICheckButtonTemplate")
frame.missing:SetSize(24, 24)
frame.missing:SetPoint("LEFT", frame.search, "RIGHT", 16, 0)
ToyboxFrameMissingText:SetText("Hide missing")

frame.prev = CreateFrame("Button", "ToyboxFramePrev", frame, "UIPanelButtonTemplate")
frame.prev:SetSize(30, 22)
frame.prev:SetText("<")
frame.prev:SetPoint("BOTTOMLEFT", 20, 18)

frame.next = CreateFrame("Button", "ToyboxFrameNext", frame, "UIPanelButtonTemplate")
frame.next:SetSize(30, 22)
frame.next:SetText(">")
frame.next:SetPoint("BOTTOMRIGHT", -20, 18)

frame.pageText = frame:CreateFontString(nil, "ARTWORK", "GameFontHighlight")
frame.pageText:SetPoint("BOTTOM", 0, 23)

local function visibleToys()
    local list = {}
    for _, toy in ipairs(ns.toys) do
        local matches = search == "" or toy.name:lower():find(search, 1, true)
        if matches and (ns.owned[toy.id] or not hideMissing) then
            list[#list + 1] = toy
        end
    end
    return list
end

local function createCell(index)
    local cell = CreateFrame("Button", "ToyboxCell" .. index, frame)
    cell:SetSize(CELL_WIDTH - 6, CELL_HEIGHT - 6)
    local column = (index - 1) % COLUMNS
    local row = math.floor((index - 1) / COLUMNS)
    cell:SetPoint("TOPLEFT", 18 + column * CELL_WIDTH, -74 - row * CELL_HEIGHT)
    cell:RegisterForClicks("LeftButtonUp", "RightButtonUp")

    cell.iconHolder = CreateFrame("Frame", nil, cell)
    cell.iconHolder:SetSize(36, 36)
    cell.iconHolder:SetPoint("LEFT")
    cell.icon = cell.iconHolder:CreateTexture(nil, "ARTWORK")
    cell.icon:SetAllPoints()
    cell.cooldown = CreateFrame("Cooldown", cell:GetName() .. "Cooldown", cell.iconHolder, "CooldownFrameTemplate")
    cell.cooldown:SetAllPoints()

    cell.name = cell:CreateFontString(nil, "ARTWORK", "GameFontNormalSmall")
    cell.name:SetPoint("LEFT", cell.iconHolder, "RIGHT", 6, 0)
    cell.name:SetPoint("RIGHT")
    cell.name:SetJustifyH("LEFT")

    cell:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    cell:SetScript("OnEnter", function(self)
        ns.SetTooltip(self, self.id)
    end)
    cell:SetScript("OnLeave", GameTooltip_Hide)
    cell:SetScript("OnClick", function(self, button)
        if not ns.owned[self.id] then
            return
        end
        if button == "RightButton" then
            ns.ToggleOnBar(self.id)
        else
            ns.Use(self.id)
        end
    end)

    cells[index] = cell
    if ns.SkinCell then
        ns.SkinCell(cell)
    end
    return cell
end

local function refresh()
    if not frame:IsShown() then
        return
    end

    local list = visibleToys()
    local pages = math.max(1, math.ceil(#list / PAGE_SIZE))
    page = math.min(page, pages)

    local count = 0
    for id in pairs(ns.owned) do
        if ns.Name(id) then
            count = count + 1
        end
    end
    frame.title:SetFormattedText("Toybox (%d/%d)", count, #ns.toys)
    frame.pageText:SetFormattedText("Page %d of %d", page, pages)
    if page > 1 then frame.prev:Enable() else frame.prev:Disable() end
    if page < pages then frame.next:Enable() else frame.next:Disable() end

    for index = 1, PAGE_SIZE do
        local toy = list[(page - 1) * PAGE_SIZE + index]
        local cell = cells[index] or createCell(index)
        if toy then
            local owned = ns.owned[toy.id]
            cell.id = toy.id
            cell.icon:SetTexture(GetItemIcon(toy.id))
            cell.icon:SetDesaturated(not owned)
            cell.iconHolder:SetAlpha(owned and 1 or 0.45)
            cell.name:SetText(toy.name)
            if owned then
                cell.name:SetTextColor(1, 0.82, 0)
            else
                cell.name:SetTextColor(0.5, 0.5, 0.5)
            end
            ns.ApplyCooldown(cell.cooldown, toy.id)
            cell:Show()
        else
            cell:Hide()
        end
    end
end

frame:SetScript("OnShow", refresh)
frame.search:SetScript("OnTextChanged", function(self)
    search = self:GetText():lower()
    page = 1
    refresh()
end)
frame.search:SetScript("OnEscapePressed", frame.search.ClearFocus)
frame.missing:SetScript("OnClick", function(self)
    hideMissing = self:GetChecked() and true or false
    page = 1
    refresh()
end)
frame.prev:SetScript("OnClick", function()
    page = page - 1
    refresh()
end)
frame.next:SetScript("OnClick", function()
    page = page + 1
    refresh()
end)
frame:EnableMouseWheel(true)
frame:SetScript("OnMouseWheel", function(_, delta)
    -- refresh limits the top end, since it knows how many pages the filters leave
    page = math.max(1, page - delta)
    refresh()
end)

ns.OnChange(refresh)

function ToyboxGrid_Toggle()
    if frame:IsShown() then
        frame:Hide()
    elseif ns.connected then
        frame:Show()
    else
        print("|cff33ff99Toybox:|r this server isn't running mod-toybox, or it's disabled.")
    end
end

SLASH_TOYBOX1 = "/toybox"
SlashCmdList.TOYBOX = function(text)
    text = text:trim()
    if text == "bar" then
        ns.ToggleBar()
    elseif text == "move" then
        ns.ToggleBarLock()
    else
        ToyboxGrid_Toggle()
    end
end
