using System;

namespace Player
{
    /// <summary>
    /// Functional category of the tool or weapon.
    /// </summary>
    public enum ToolType
    {
        None = 0,
        Pickaxe,  // Mines stone, ores, bricks at high speed
        Axe,      // Rapid chop on wood, planks, leaves, cactus
        Sword,    // Heavy melee damage and massive knockback against mobs
        Shovel    // Rapid dig on dirt, sand, clay, snow, mud
    }

    /// <summary>
    /// Material quality tier of the tool.
    /// Determines damage bonuses, efficiency multipliers, and knockback force.
    /// </summary>
    public enum ToolTier
    {
        None = 0,
        Wood,     // Starter wooden tier
        Stone,    // Sturdy stone tier
        Iron,     // Hardened iron tier
        Diamond   // Supreme diamond tier
    }

    /// <summary>
    /// Distinct Unique Identifiers for all 16 tools across Wood, Stone, Iron, and Diamond tiers.
    /// Uses 101+ ID range to cleanly avoid collisions with BlockType (0-26).
    /// </summary>
    public enum ToolID
    {
        None = 0,

        // Pickaxes (101 - 104)
        WoodPickaxe    = 101,
        StonePickaxe   = 102,
        IronPickaxe    = 103,
        DiamondPickaxe = 104,

        // Axes (105 - 108)
        WoodAxe        = 105,
        StoneAxe       = 106,
        IronAxe        = 107,
        DiamondAxe     = 108,

        // Swords (109 - 112)
        WoodSword      = 109,
        StoneSword     = 110,
        IronSword      = 111,
        DiamondSword   = 112,

        // Shovels (113 - 116)
        WoodShovel     = 113,
        StoneShovel    = 114,
        IronShovel     = 115,
        DiamondShovel  = 116
    }
}
