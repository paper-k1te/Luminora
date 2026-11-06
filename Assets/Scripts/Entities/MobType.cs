namespace Entities
{
    /// <summary>
    /// Distinct 2.5D mob varieties inspired by Luminora and Minecraft.
    /// </summary>
    public enum MobType
    {
        /// <summary>
        /// Verdant Forest / Grasslands jumper. Squashes on land, stretches in air.
        /// Drops Gel / Slime Ball.
        /// </summary>
        Slime = 0,

        /// <summary>
        /// Underground Cavern flyer. Flaps articulated wings, glides along smooth sine waves, swoops down.
        /// Drops Coal Ore / Ruby.
        /// </summary>
        Bat = 1,

        /// <summary>
        /// Night surface and deep underworld walker. Outstretched horizontal arms, auto 1-block step climbing.
        /// Drops Iron Ore / Gold Nugget.
        /// </summary>
        Zombie = 2
    }
}
