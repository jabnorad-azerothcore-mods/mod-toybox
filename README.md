# mod-toybox

A retail-style, account-wide toybox for AzerothCore (WoW 3.3.5a): a server module plus a
client addon. Toys no longer take up bag space: using one adds it to a toybox shared by
every character on the account, and it's used from there, with its original cooldown.

## Install

This needs a recent AzerothCore: the module is built against current AzerothCore master,
and older cores may not compile it.

Back up `acore_auth`, `acore_world` and `acore_characters` first.

### Server

1. Clone this repo into the core's `modules/` folder, as `modules/mod-toybox`.
2. Rebuild. With the Docker setup, run `docker compose up -d --build` from the core's root,
   which rebuilds worldserver and db-import and applies the module's SQL. On a manual install,
   re-run CMake, compile, and copy `mod_toybox.conf.dist` next to your other module confs.
3. Copy `mod_toybox.conf.dist` to `mod_toybox.conf` in `env/dist/etc/modules/`, set
   `Toybox.Enable = 1`, and restart worldserver. The module does nothing until then.
4. Check that the worldserver log says `mod-toybox: loaded 63 toys`.

### Client

Copy `addon/Toybox` to `Interface/AddOns`, or unzip `Toybox.zip` from the
[latest release](https://github.com/jabnorad-azerothcore-mods/mod-toybox/releases/latest) there. It needs no other
addons, and matches ElvUI when that's installed.

## Use

| | |
|---|---|
| `/toybox` | Open the window (or set a keybind). Click a toy to use it, right-click to add it to the toy bar |
| `/toybox bar` | Show or hide the toy bar |
| `/toybox move` | Unlock the toy bar to drag it; run again to lock |
| `/toy <name or ID>` | Use a toy, for macros |
| `.toybox list` | How many toys you've collected |
| `.toybox use <ID>` | Use a toy without the addon |
| `.toybox link <account> <account>` | GM: share one toybox between accounts, merging their toys. Needs `Toybox.SharedAccounts.Enable` |
| `.toybox unlink <account>` | GM: give an account its own empty toybox again |

Toys keep their original cooldowns, reagents, and class, race, level and skill limits. Cooldowns are
per character. Toys that need a ground target are placed at your feet.

Deleting an account keeps its toybox while another account still shares it. Toys go with the
last account using them.

Macros on normal action bars work, but they can't show a toy's cooldown; the toy bar does.

## Changing the toy list

The default list is a snapshot of retail's toys that exist in 3.3.5a, and isn't kept current.
[docs/toy-list.md](docs/toy-list.md) shows what ships and why. The list is the
`mod_toybox_item` world table. Items that can be equipped or have limited charges are rejected. `Toybox.ExtraItems` and
`Toybox.ExcludedItems` in `mod_toybox.conf` adjust it without touching SQL, so a server's
additions survive module updates. A restart applies changes.
