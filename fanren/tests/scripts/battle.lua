-- 战斗往返。胜负走不同分支。
if battle("t.wild_dog") then
    flag.set("battle_won")
    talk("hanli", "t.victory")
else
    flag.set("battle_lost")
    talk("hanli", "t.defeat")
end
