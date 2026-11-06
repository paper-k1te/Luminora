namespace Core
{
    /// <summary>
    /// Identifiers for all block and tile types in the voxel world.
    /// Uses byte backing type for minimal memory footprint in large chunk arrays.
    /// </summary>
    public enum BlockType : byte
    {
        Air = 0,
        Grass = 1,
        Dirt = 2,
        Stone = 3,
        Wood = 4,
        Leaves = 5,
        Sand = 6,
        Sandstone = 7,
        Cactus = 8,
        Snow = 9,
        Ice = 10,
        Clay = 11,
        CoalOre = 12,
        IronOre = 13,
        GoldOre = 14,
        DiamondOre = 15,
        RubyOre = 16,
        Brick = 17,
        WoodPlank = 18,
        Glass = 19,
        Torch = 20,
        Obsidian = 21,
        Magma = 22,
        MossyStone = 23,
        Mud = 24,
        GlowShroom = 25,
        Bedrock = 26,

        // Craftable Tools, Weapons & Materials
        Stick = 27,
        WoodenPickaxe = 28,
        WoodenAxe = 29,
        WoodenSword = 30,
        WoodenShovel = 31,
        StonePickaxe = 32,
        StoneAxe = 33,
        StoneSword = 34,
        IronPickaxe = 35,
        IronAxe = 36,
        IronSword = 37,
        DiamondPickaxe = 38,
        DiamondSword = 39,
        Gel = 40,
        StickyBomb = 41
    }
}
