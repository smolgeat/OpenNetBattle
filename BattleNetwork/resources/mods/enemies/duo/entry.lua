nonce = function() end 

function LoadTexture(path)
    return Engine.ResourceHandle.new().Textures:LoadFile(path)
end

function StreamMusic(path, loop)
    return Engine.ResourceHandle.new().Audio:Stream(path, loop)
end

local texture = nil
local shake = false
local waitTime = 1
local hand = nil
local middle = nil
local miscAnim = nil
local anim = nil
local dir = Direction.Up

local aiStateIndex = 1 
local aiState = {} -- setup in the battle_init function

local MINE_SLIDE_FRAMES = 20 -- frames

function Mine() 
    local mine = Battle.Obstacle.new(Team.Blue)
    mine:SetHealth(20)
    mine:SetSlideFrames(MINE_SLIDE_FRAMES)

    mine.dir = Direction.Up
    mine.verticalDir = Direction.Up
    mine.moveOneColumn = false
    mine.timer = 0

    mine.node = Engine.SpriteNode.new()
    mine.node:SetTexture(texture, true)
    mine.node:SetPosition(0,0)
    mine:AddNode(mine.node)

    miscAnim:SetState("MINE", Playback.Once, nonce)
    miscAnim:Refresh(mine.node:Sprite())

    mine.updateFunc = function(self, dt) 
        local tile = self:Tile()

        -- every frame try to attack shared tile
        tile:AttackEntities(self)

        local tileW = tile:Width()/2.0 -- downscale 2x
        local offset = math.min(1.0, ((self.timer*60.0)/((MINE_SLIDE_FRAMES*2.0)+1.0)))
        local x = (-offset*tileW)+(tileW/2.0)

        self.node:SetPosition(x, 0.0)
        self.timer = self.timer + dt

        if self:IsSliding() == false then
            local nextTile = nil
            
            if self.dir == Direction.Up then
                nextTile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()-1)
            elseif self.dir == Direction.Down then 
                nextTile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()+1)
            end

            if nextTile == nil then 
                self:Delete()
                return
            end

            if nextTile:IsEdge() and self.moveOneColumn == false then
                    -- we must be moving up/down
                    self.moveOneColumn = true
                    self.timer = 0
                    self.node:SetPosition(tileW/2.0, 0.0)

                    if not self:Move(Direction.Left) then 
                        self:Delete()
                        return
                    end

                    self:AdoptNextTile()
                    self:FinishMove()
                    return
            elseif self.moveOneColumn == true then
                if self.verticalDir == Direction.Up then
                    self.verticalDir = Direction.Down
                    self.dir = Direction.Down 
                else 
                    self.dir = Direction.Up
                    self.verticalDir = Direction.Up 
                end

                self.moveOneColumn = false
            end

            -- otherwise keep sliding
            self:SlideToTile(true)
            self:Move(self.dir)
        end 
    end

    mine.attackFunc = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
        self:Delete()
    end

    mine.deleteFunc = function(self) 
        -- nothing
    end

    mine.canMoveToFunc = function(tile)
        -- mines fly over any tiles
        return true
    end

    return mine
end

function Missile()
    local missile = Battle.Obstacle.new(Team.Blue)
    missile:SetHealth(20)
    missile:SetTexture(texture, true)
    missile:SetSlideFrames(30)
    missile:SetHeight(20.0)
    missile:SetLayer(-2)
    missile:ShowShadow(false)

    local missileAnim = missile:GetAnimation()
    missileAnim:CopyFrom(anim)
    missileAnim:SetState("MISSILE", Playback.Loop, nonce)

    missile.updateFunc = function(self, dt) 

        if self:IsSliding() == false then
            if self:Tile():IsEdge() then 
                self:Delete()
            end

            -- otherwise keep sliding
            self:SlideToTile(true)
            self:Move(Direction.Left)
        end 

        if self:Tile():IsHole() == false then 
            missile:ShowShadow(true)
        end

        -- every frame try to attack shared tile
        self:Tile():AttackEntities(self)
    end

    missile.attackFunc = function(self, other) 
        local explosion = Battle.Explosion.new(1, 1)
        self:Field():SpawnFX(explosion, self:Tile():X(), self:Tile():Y())
        self:Delete()
    end

    missile.deleteFunc = function(self) 
        -- nothing
    end

    missile.canMoveToFunc = function(tile)
        -- missiles fly over any tiles
        return true
    end

    return missile
end

function battle_init(self) 
    aiState = {}
    aiStateIndex = 1
    aiState[1] = Missile
    aiState[2] = Mine

    texture = LoadTexture(_modpath.."duo_compressed.png")

    print("modpath: ".._modpath)
    self:SetName("Duo")
    self:SetHealth(3000)
    self:SetTexture(texture, true)
    self:SetHeight(60)
    self:SetSlideFrames(120)
    self:ShareTile(true)

    anim = self:GetAnimation()
    anim:SetPath(_modpath.."duo_compressed.animation")
    anim:Load()
    anim:SetState("BODY", Playback.Once, nonce)

    hand = Engine.SpriteNode.new()
    hand:SetTexture(texture, true)
    hand:SetPosition(0, 15.0)
    hand:SetLayer(-2) -- put it in front at all times
    self:AddNode(hand)

    miscAnim = Engine.Animation.new(anim:Copy())
    miscAnim:SetState("HAND_IDLE")
    miscAnim:Refresh(hand:Sprite())

    middle = Engine.SpriteNode.new()
    middle:SetTexture(texture, true)
    middle:SetLayer(-1)

    miscAnim:SetState("NO_SHOOT")
    miscAnim:Refresh(middle:Sprite())

    local origin = anim:Point("origin")
    local point  = anim:Point("NO_SHOOT")
    middle:SetPosition(point.x - origin.x, point.y - origin.y)
    self:AddNode(middle)

    print("done")
end

function num_of_explosions()
    return 12
end

function on_update(self, dt)
    if self:IsSliding() == false then
        -- we arrive, shake the ground
        
        if shake then
            self:ShakeCamera(3.0, 1.0)
            shake = false
            middle:Hide() -- reveal red center
        end
        
        waitTime = waitTime - dt
        
        -- pause before moving again
        if waitTime <= 0 then
            -- use AI state to determine next move 
            self:Field():Spawn(aiState[aiStateIndex](), self:Tile():X(), self:Tile():Y())
            print("field spawn")

            aiStateIndex = aiStateIndex + 1
            if aiStateIndex > #aiState then 
                aiStateIndex = 1
            end

            local tile = nil
            
            if dir == Direction.Up then 
                tile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()-1)

                if can_move_to(tile) == false then 
                    dir = Direction.Down
                end
            elseif dir == Direction.Down then
                tile = self:Field():TileAt(self:Tile():X(), self:Tile():Y()+1)

                if can_move_to(tile) == false then
                    dir = Direction.Up
                end
            end

            waitTime = 1
            shake = true -- can shake again
            middle:Show() -- coverup red center
            self:SlideToTile(true)
            self:Move(dir)
        end
    end 
end

function on_battle_start(self)
   print("on_battle_start called")
end

function on_battle_end(self)
    print("on_battle_end called")
end

function on_spawn(self, tile) 
    StreamMusic(_modpath.."music.ogg", true)
    print("on_spawn called")
end

function can_move_to(tile) 
    if tile:IsEdge() then
        return false
    end

    return true
end