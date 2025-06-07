IncludeScript("base_shutdown");

-----------------------------------------------------------------------------
-- Clips on Spawns
-----------------------------------------------------------------------------
clip_brush = trigger_ff_clip:new({ clipflags = 0 })

blue_block = clip_brush:new({ clipflags = {ClipFlags.kClipPlayers, ClipFlags.kClipTeamBlue} })
red_block = clip_brush:new({ clipflags = {ClipFlags.kClipPlayers, ClipFlags.kClipTeamRed} })

-----------------------------------------------------------------------------
-- backpacks
-----------------------------------------------------------------------------
red_pack = genericbackpack:new({
	grenades = 100,
	bullets = 100,
	nails = 100,
	shells = 100,
	rockets = 100,
	gren1 = 0,
	gren2 = 0,
      cells = 100,
	armor = 40,
	health = 50,
      respawntime = 16,
	model = "models/items/backpack/backpack.mdl",
	materializesound = "Item.Materialize",
	touchsound = "Backpack.Touch",
	touchflags = {AllowFlags.kRed},
	botgoaltype = Bot.kBackPack_Ammo
})

function red_pack:dropatspawn() return false end

blue_pack = genericbackpack:new({
	grenades = 100,
	bullets = 100,
	nails = 100,
	shells = 100,
	rockets = 100,
	gren1 = 0,
	gren2 = 0,
      cells = 100,
	armor = 40,
	health = 50,
      respawntime = 16,
	model = "models/items/backpack/backpack.mdl",
	materializesound = "Item.Materialize",
	touchsound = "Backpack.Touch",
	touchflags = {AllowFlags.kBlue},
	botgoaltype = Bot.kBackPack_Ammo
})

function blue_pack:dropatspawn() return false end

resup_pack = genericbackpack:new({
	grenades = 100,
	bullets = 100,
	nails = 100,
	shells = 100,
	rockets = 100,
	gren1 = 0,
	gren2 = 0,
	cells = 100,
	armor = 40,
	health = 50,
	respawntime = 16,
	model = "models/items/backpack/backpack.mdl",
	materializesound = "Item.Materialize",
	touchsound = "Backpack.Touch",
})

function resup_pack:dropatspawn() return false end

-----------------------------------------------------------------------------
-- Spawns
-----------------------------------------------------------------------------
redspawn_offense = function(self,player) return ((player:GetTeamId() == Team.kRed) and ((player:GetClass() == Player.kScout) or (player:GetClass() == Player.kMedic) or (player:GetClass() == Player.kSpy))) end
redspawn_defense = function(self,player) return ((player:GetTeamId() == Team.kRed) and ((player:GetClass() == Player.kDemoman) or (player:GetClass() == Player.kPyro) or (player:GetClass() == Player.kSniper ) or (player:GetClass() == Player.kHwguy))) end
redspawn_defense_soldier = function(self,player) return ((player:GetTeamId() == Team.kRed ) and (player:GetClass() == Player.kSoldier)) end
redspawn_defense_engineer = function(self,player) return ((player:GetTeamId() == Team.kRed ) and (player:GetClass() == Player.kEngineer)) end

bluespawn_offense = function(self,player) return ((player:GetTeamId() == Team.kBlue) and ((player:GetClass() == Player.kScout) or (player:GetClass() == Player.kMedic) or (player:GetClass() == Player.kSpy))) end
bluespawn_defense = function(self,player) return ((player:GetTeamId() == Team.kBlue) and ((player:GetClass() == Player.kDemoman) or (player:GetClass() == Player.kPyro) or (player:GetClass() == Player.kSniper ) or (player:GetClass() == Player.kHwguy))) end
bluespawn_defense_soldier = function(self,player) return ((player:GetTeamId() == Team.kBlue ) and (player:GetClass() == Player.kSoldier)) end
bluespawn_defense_engineer = function(self,player) return ((player:GetTeamId() == Team.kBlue ) and (player:GetClass() == Player.kEngineer)) end

redspawn_offense = { validspawn = redspawn_offense }
redspawn_defense  = { validspawn = redspawn_defense }
redspawn_defense_soldier = { validspawn = redspawn_defense_soldier }
redspawn_defense_engineer = { validspawn = redspawn_defense_engineer }

bluespawn_offense = { validspawn = bluespawn_offense }
bluespawn_defense  = { validspawn = bluespawn_defense }
bluespawn_defense_soldier = { validspawn = bluespawn_defense_soldier }
bluespawn_defense_engineer = { validspawn = bluespawn_defense_engineer }

