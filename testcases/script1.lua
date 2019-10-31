--[[TeXworksScript
Title: Tw.insertText test
Description: This is a unicode string 🤩
Author: Stefan Löffler
Version: 0.0.1
Date: 2019-10-22
Script-Type: standalone
Context: TeXDocument
Shortcut: Ctrl+Alt+Shift+I
]]

if TW.script.getGlobal("TwNil") ~= nil then error("invalid nil") end
if TW.script.getGlobal("TwBool") ~= true then error("invalid bool") end
if TW.script.getGlobal("TwDouble") ~= 4.2 then error("invalid double") end
if TW.script.getGlobal("TwString") ~= "Ok" then error("invalid string") end

l = TW.script.getGlobal("TwList")
if l[1] ~= "Fourty" then error("invalid list") end
if l[2] ~= 2 then error("invalid list") end

m = TW.script.getGlobal("TwMap")
if m["k"] ~= "v" then error("invalid map") end

h = TW.script.getGlobal("TwHash")
if h["k"] ~= "v" then error("invalid hash") end

TW.target.insertText("It works!")

TW.script.setGlobal("LuaNil", nil)
TW.script.setGlobal("LuaBool", true)
TW.script.setGlobal("LuaArray", {"Hello", 42, false})
TW.script.setGlobal("LuaMap", {key = "value", [1] = 0})
TW.script.setGlobal("LuaQObject*", TW)

TW.result = {1, 2, 3}
