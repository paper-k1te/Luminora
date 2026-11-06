using System;
using System.Collections.Generic;
using Core;
using Player;
using UnityEngine;

namespace Crafting
{
    /// <summary>
    /// Represents a single ingredient requirement for a crafting recipe.
    /// </summary>
    [Serializable]
    public class IngredientRequirement
    {
        public BlockType itemType;
        public int amount;

        public IngredientRequirement()
        {
            itemType = BlockType.Air;
            amount = 1;
        }

        public IngredientRequirement(BlockType itemType, int amount)
        {
            this.itemType = itemType;
            this.amount = amount;
        }
    }

    /// <summary>
    /// Recipe definition metadata and evaluation engine.
    /// Defines input requirements, craftable output products, categorization,
    /// and inventory deduction routines for both Survival and Creative modes.
    /// </summary>
    [Serializable]
    public class RecipeDefinition
    {
        [Header("Recipe Identity")]
        public int id;
        public string recipeName;
        public string chineseName;
        public string category; // "Tools", "Weapons", "Blocks", "Supplies"
        [TextArea(2, 4)]
        public string description;

        public string DisplayName => LocalizationManager.Get(chineseName, recipeName);
        public string DisplayCategory
        {
            get
            {
                switch (category)
                {
                    case "Tools": return LocalizationManager.Get("工具", "Tools");
                    case "Weapons": return LocalizationManager.Get("武器", "Weapons");
                    case "Blocks": return LocalizationManager.Get("建筑", "Blocks");
                    case "Supplies": return LocalizationManager.Get("消耗", "Supplies");
                    case "Materials": return LocalizationManager.Get("材料", "Materials");
                    default: return category;
                }
            }
        }

        [Header("Ingredients & Result")]
        public IngredientRequirement[] ingredients;
        public BlockType resultItem;
        public int resultCount = 1;
        public string iconEmoji = "🔨";

        public RecipeDefinition()
        {
            ingredients = new IngredientRequirement[0];
            resultCount = 1;
        }

        public RecipeDefinition(
            int id,
            string recipeName,
            string chineseName,
            string category,
            string description,
            BlockType resultItem,
            int resultCount,
            string iconEmoji,
            params IngredientRequirement[] ingredients)
        {
            this.id = id;
            this.recipeName = recipeName;
            this.chineseName = chineseName;
            this.category = category;
            this.description = description;
            this.resultItem = resultItem;
            this.resultCount = resultCount;
            this.iconEmoji = iconEmoji;
            this.ingredients = ingredients ?? new IngredientRequirement[0];
        }

        /// <summary>
        /// Scans all inventory and hotbar slots to count available quantity of an item.
        /// </summary>
        public int GetPlayerItemCount(InventorySystem inventory, BlockType type)
        {
            if (inventory == null) return 0;
            int total = 0;
            for (int i = 0; i < inventory.SlotCount; i++)
            {
                InventorySlot slot = inventory.GetSlot(i);
                if (slot != null && !slot.IsEmpty && slot.type == type)
                {
                    total += slot.count;
                }
            }
            return total;
        }

        /// <summary>
        /// Checks whether the player currently holds all required ingredients in Survival,
        /// or allows instant free crafting in Creative mode.
        /// </summary>
        public bool CanCraft(InventorySystem inventory, bool isCreative)
        {
            if (isCreative) return true;
            if (inventory == null || ingredients == null || ingredients.Length == 0) return false;

            for (int i = 0; i < ingredients.Length; i++)
            {
                IngredientRequirement req = ingredients[i];
                if (req.amount <= 0) continue;
                if (GetPlayerItemCount(inventory, req.itemType) < req.amount)
                {
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Calculates the maximum number of times this recipe can be crafted with current materials.
        /// </summary>
        public int GetMaxCraftableCount(InventorySystem inventory, bool isCreative)
        {
            if (isCreative) return 999;
            if (inventory == null || ingredients == null || ingredients.Length == 0) return 0;

            int maxCount = int.MaxValue;
            for (int i = 0; i < ingredients.Length; i++)
            {
                IngredientRequirement req = ingredients[i];
                if (req.amount <= 0) continue;
                int owned = GetPlayerItemCount(inventory, req.itemType);
                int craftableWithThis = owned / req.amount;
                if (craftableWithThis < maxCount)
                {
                    maxCount = craftableWithThis;
                }
            }
            return maxCount == int.MaxValue ? 0 : maxCount;
        }

        /// <summary>
        /// Deducts the required ingredients from player inventory slots (backpack + hotbar).
        /// Returns true if all materials were successfully deducted.
        /// </summary>
        public bool DeductIngredients(InventorySystem inventory, int craftMultiplier = 1)
        {
            if (inventory == null || ingredients == null || craftMultiplier <= 0) return false;

            // Verify first before modifying
            for (int i = 0; i < ingredients.Length; i++)
            {
                IngredientRequirement req = ingredients[i];
                int totalNeeded = req.amount * craftMultiplier;
                if (GetPlayerItemCount(inventory, req.itemType) < totalNeeded)
                {
                    return false;
                }
            }

            // Perform deductions
            for (int i = 0; i < ingredients.Length; i++)
            {
                IngredientRequirement req = ingredients[i];
                int remainingToDeduct = req.amount * craftMultiplier;

                for (int slotIdx = 0; slotIdx < inventory.SlotCount && remainingToDeduct > 0; slotIdx++)
                {
                    InventorySlot slot = inventory.GetSlot(slotIdx);
                    if (slot != null && !slot.IsEmpty && slot.type == req.itemType)
                    {
                        if (slot.count <= remainingToDeduct)
                        {
                            remainingToDeduct -= slot.count;
                            inventory.RemoveItem(slotIdx, slot.count);
                        }
                        else
                        {
                            inventory.RemoveItem(slotIdx, remainingToDeduct);
                            remainingToDeduct = 0;
                        }
                    }
                }
            }

            return true;
        }
    }
}
