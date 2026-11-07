-- Matches ElvUI when it's loaded, and does nothing otherwise. Only ElvUI's public skinning
-- functions are used, so another build only needs changes here
local _, ns = ...

local E, S

local function skinIcon(holder, icon)
    if E.TexCoords then
        icon:SetTexCoord(unpack(E.TexCoords))
    end
    if holder.SetTemplate then
        holder:SetTemplate("Default")
        icon:SetDrawLayer("OVERLAY")
        icon:ClearAllPoints()
        icon:SetPoint("TOPLEFT", holder, "TOPLEFT", E.Border or 1, -(E.Border or 1))
        icon:SetPoint("BOTTOMRIGHT", holder, "BOTTOMRIGHT", -(E.Border or 1), E.Border or 1)
    end
end

local function registerCooldown(cooldown)
    if E.RegisterCooldown then
        E:RegisterCooldown(cooldown)
    end
end

function ns.SkinCell(cell)
    if E then
        skinIcon(cell.iconHolder, cell.icon)
        registerCooldown(cell.cooldown)
    end
end

function ns.SkinBarButton(button)
    if E then
        skinIcon(button, button.icon)
        registerCooldown(button.cooldown)
    end
end

local function skinWindow()
    local frame = ns.frame
    if frame.SetTemplate then
        frame:SetBackdrop(nil)
        frame:SetTemplate("Transparent")
    end
    S:HandleCloseButton(frame.close)
    S:HandleEditBox(frame.search)
    S:HandleCheckBox(frame.missing)
    S:HandleButton(frame.prev)
    S:HandleButton(frame.next)
end

local events = CreateFrame("Frame")
events:RegisterEvent("PLAYER_LOGIN")
events:SetScript("OnEvent", function()
    if not ElvUI then
        return
    end
    E = unpack(ElvUI)
    S = E and E.GetModule and E:GetModule("Skins", true)
    if S then
        skinWindow()
    else
        E = nil
    end
end)
