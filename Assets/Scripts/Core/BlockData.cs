using System;
using UnityEngine;

namespace Core
{
    /// <summary>
    /// Metadata definition for physical, rendering, and interaction properties of a block.
    /// </summary>
    [Serializable]
    public class BlockData
    {
        [Header("Identity")]
        public BlockType type;
        public string chineseName;
        public string englishName;
        [SerializeField] private string _displayName;

        /// <summary>
        /// Localized display name dynamically queried from LocalizationManager.
        /// </summary>
        public string displayName
        {
            get
            {
                if (!string.IsNullOrEmpty(chineseName) && !string.IsNullOrEmpty(englishName))
                {
                    return LocalizationManager.Get(chineseName, englishName);
                }
                return _displayName ?? chineseName ?? englishName ?? type.ToString();
            }
            set => _displayName = value;
        }

        [Header("Physical Attributes")]
        public float hardness;       // Time / hits required to break (negative for unbreakable)
        public bool isSolid;         // Determines if blocks have physical collision
        public bool isPassable;      // True for non-solid or trigger blocks (torches, flora)

        [Header("Rendering & Visibility")]
        public bool isTransparent;   // True if light passes through and adjacent block faces should render
        public int textureIndex;     // Tile coordinate/index in the master texture atlas
        public Color defaultColor;   // Fallback color tint or base color for untextured meshes

        [Header("Illumination")]
        public bool emitLight;       // Does this block emit voxel light?
        public Color lightColor;     // Emitted RGB light color and intensity tint

        [Header("Loot & Drops")]
        public int dropItemId;       // ID of item or BlockType dropped on destruction
        public int dropCount;        // Default item quantity dropped

        public BlockData()
        {
            type = BlockType.Air;
            chineseName = "空气";
            englishName = "Air";
            _displayName = "Air";
            hardness = 0f;
            isSolid = false;
            isPassable = true;
            isTransparent = true;
            textureIndex = 0;
            defaultColor = Color.clear;
            emitLight = false;
            lightColor = Color.black;
            dropItemId = 0;
            dropCount = 0;
        }

        public BlockData(
            BlockType type,
            string chineseName,
            string englishName,
            float hardness,
            bool isSolid,
            bool isPassable,
            bool isTransparent,
            int textureIndex,
            Color defaultColor,
            bool emitLight = false,
            Color? lightColor = null,
            int dropItemId = -1,
            int dropCount = 1)
        {
            this.type = type;
            this.chineseName = chineseName;
            this.englishName = englishName;
            this._displayName = englishName;
            this.hardness = hardness;
            this.isSolid = isSolid;
            this.isPassable = isPassable;
            this.isTransparent = isTransparent;
            this.textureIndex = textureIndex;
            this.defaultColor = defaultColor;
            this.emitLight = emitLight;
            this.lightColor = lightColor ?? (emitLight ? Color.white : Color.black);
            this.dropItemId = dropItemId >= 0 ? dropItemId : (int)type;
            this.dropCount = dropCount;
        }

        public BlockData(
            BlockType type,
            string displayName,
            float hardness,
            bool isSolid,
            bool isPassable,
            bool isTransparent,
            int textureIndex,
            Color defaultColor,
            bool emitLight = false,
            Color? lightColor = null,
            int dropItemId = -1,
            int dropCount = 1)
            : this(type, displayName, displayName, hardness, isSolid, isPassable, isTransparent, textureIndex, defaultColor, emitLight, lightColor, dropItemId, dropCount)
        {
        }

        /// <summary>
        /// Explicit helper returning the localized name.
        /// </summary>
        public string GetDisplayName() => displayName;

        /// <summary>
        /// True if this block constitutes an active, existing tile (not air).
        /// </summary>
        public bool IsActive => type != BlockType.Air;
    }
}
