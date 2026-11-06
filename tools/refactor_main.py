import os

main_path = os.path.join('standalone_src', 'main.c')
with open(main_path, 'r', encoding='utf-8') as f:
    content = f.read()

# Normalize to LF for easy matching
c = content.replace('\r\n', '\n')

# 1. Clean up old commented biome block if present
old_biome_comment = '''/*
    "Sunken Desert (干燥沙漠)",
    "Verdant Forest (温带森林)",
    "Glacial Tundra (极地雪原)"
};'''
if old_biome_comment in c:
    c = c.replace(old_biome_comment, '')

# 2. Update saveGame & loadGame
old_save = '''    fclose(f);
    showNotification("World Saved! [F5] (存档已保存)", 0.2f, 0.95f, 0.4f);
    return true;'''

new_save = '''    int currentLang = (int)getLanguage();
    fwrite(&currentLang, sizeof(int), 1, f);

    fclose(f);
    showNotification(LOC("世界保存成功！[F5]", "World Saved! [F5]"), 0.2f, 0.95f, 0.4f);
    return true;'''

assert old_save in c, "old_save not found"
c = c.replace(old_save, new_save, 1)

old_load = '''    fclose(f);
    g_CamX = g_Player.x;
    g_CamY = g_Player.y;
    showNotification("World Loaded! [F9] (存档读取成功)", 0.3f, 0.85f, 1.0f);
    return true;'''

new_load = '''    int savedLang = 0;
    if (fread(&savedLang, sizeof(int), 1, f) == 1) {
        if (savedLang == LANG_ZH || savedLang == LANG_EN) {
            setLanguage((Language)savedLang);
        }
    }

    fclose(f);
    g_CamX = g_Player.x;
    g_CamY = g_Player.y;
    showNotification(LOC("世界读取成功！[F9]", "World Loaded! [F9]"), 0.3f, 0.85f, 1.0f);
    return true;'''

assert old_load in c, "old_load not found"
c = c.replace(old_load, new_load, 1)

# loadGame failure notifications
c = c.replace('showNotification("No save file found! (未找到存档)", 0.95f, 0.3f, 0.2f);',
              'showNotification(LOC("未找到世界存档文件！", "No save file found!"), 0.95f, 0.3f, 0.2f);')
c = c.replace('showNotification("Save file corrupted! (存档损坏)", 0.95f, 0.2f, 0.2f);',
              'showNotification(LOC("世界存档文件已损坏！", "Save file corrupted!"), 0.95f, 0.2f, 0.2f);')

# 3. Add to inventory floating text
old_add_inv = 'snprintf(buf, sizeof(buf), "+%d %s", count, g_BlockDefs[type].chineseName);'
new_add_inv = 'snprintf(buf, sizeof(buf), "+%d %s", count, (getLanguage() == LANG_ZH) ? g_BlockDefs[type].chineseName : g_BlockDefs[type].name);'
assert old_add_inv in c, "old_add_inv not found"
c = c.replace(old_add_inv, new_add_inv, 1)

# 4. In-game notifications
c = c.replace('showNotification("Welcome to Luminora! Press [H] for Help, [F5] to Save.", 1.0f, 0.85f, 0.3f);',
              'showNotification(LOC("欢迎来到 Luminora｜流明洛原！按 [H] 查看指南，[F5] 快速保存。", "Welcome to Luminora! Press [H] for Help, [F5] to Save."), 1.0f, 0.85f, 0.3f);')

c = c.replace('showNotification("Quest 1 Completed! Now mine 5 blocks with Left-Click.", 0.3f, 0.95f, 0.3f);',
              'showNotification(LOC("任务 1 已完成！按住鼠标左键挖掘 5 个方块。", "Quest 1 Completed! Now mine 5 blocks with Left-Click."), 0.3f, 0.95f, 0.3f);')

c = c.replace('showNotification("Quest 2 Completed! Now select a block [1-9] & Right-Click to place.", 0.3f, 0.95f, 0.3f);',
              'showNotification(LOC("任务 2 已完成！选中方块 [1-9]，按鼠标右键放置。", "Quest 2 Completed! Select a block [1-9] & Right-Click to place."), 0.3f, 0.95f, 0.3f);')

c = c.replace('showNotification("Quest 3 Completed! Select Torch [5] and place it to light dark caves.", 0.3f, 0.95f, 0.3f);',
              'showNotification(LOC("任务 3 已完成！选定火把 [5] 并放置以照亮洞穴。", "Quest 3 Completed! Select Torch [5] and place to light dark caves."), 0.3f, 0.95f, 0.3f);')

c = c.replace('showNotification("Quest 4 Completed! Press [F5] to Quick Save your game!", 0.3f, 0.95f, 0.3f);',
              'showNotification(LOC("任务 4 已完成！按 [F5] 快速保存你的世界！", "Quest 4 Completed! Press [F5] to Quick Save your game!"), 0.3f, 0.95f, 0.3f);')

c = c.replace('showNotification("Respawned on surface safe ground! (已安全重生)", 0.4f, 0.8f, 1.0f);',
              'showNotification(LOC("已在地面安全区域重生！", "Respawned on surface safe ground!"), 0.4f, 0.8f, 1.0f);')

# 5. Depth and status panel in renderHUD()
old_depth = '''    const char* depthName = (g_Player.y < UNDERWORLD_LEVEL) ? "Zone: Underworld Magma (深层熔岩带)" :
                            ((g_Player.y < CAVERN_LEVEL) ? "Zone: Deep Caverns (地下深渊)" :
                            ((g_Player.y < SUBSURFACE_LEVEL) ? "Zone: Subsurface (地下土层)" : "Zone: Surface Ground (地表世界)"));
    drawText(25, 68, depthName, 0.7f, 0.8f, 0.85f, false);

    char modeBadge[128];
    if (getGameMode() == MODE_CREATIVE) {
        snprintf(modeBadge, sizeof(modeBadge), "Mode: CREATIVE %s", isCreativeFlying() ? "[⚡ FLYING]" : "[FLY: 2x SPACE]");
        drawText(25, 86, modeBadge, isCreativeFlying() ? 0.35f : 1.0f, 0.90f, isCreativeFlying() ? 1.0f : 0.3f, true);
    } else {
        snprintf(modeBadge, sizeof(modeBadge), "Mode: SURVIVAL (生存模式)");
        drawText(25, 86, modeBadge, 0.4f, 0.95f, 0.5f, false);
    }
    drawText(25, 102, "[ESC] Pause Menu | [F5] Save | [F9] Load | [H] Help", 0.65f, 0.75f, 0.85f, false);'''

new_depth = '''    const char* depthName = (g_Player.y <= BEDROCK_LEVEL) ? LOC("基岩地层", "Bedrock") :
                            ((g_Player.y < UNDERWORLD_LEVEL) ? LOC("熔岩地狱", "Underworld") :
                            ((g_Player.y < CAVERN_LEVEL) ? LOC("地下矿洞", "Cavern") :
                            ((g_Player.y < SUBSURFACE_LEVEL) ? LOC("地下土层", "Subsurface") :
                            LOC("地表世界", "Surface"))));
    char depthBuf[64];
    snprintf(depthBuf, sizeof(depthBuf), "%s: %s", LOC("区域", "Zone"), depthName);
    drawText(25, 68, depthBuf, 0.7f, 0.8f, 0.85f, false);

    char modeBadge[128];
    if (getGameMode() == MODE_CREATIVE) {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s %s",
            LOC("当前模式", "Mode"), LOC("创造模式", "Creative"),
            isCreativeFlying() ? LOC("[飞行开启]", "[FLYING]") : LOC("[双击空格飞行]", "[2x SPACE]"));
        drawText(25, 86, modeBadge, isCreativeFlying() ? 0.35f : 1.0f, 0.90f, isCreativeFlying() ? 1.0f : 0.3f, true);
    } else {
        snprintf(modeBadge, sizeof(modeBadge), "%s: %s", LOC("当前模式", "Mode"), LOC("生存模式", "Survival"));
        drawText(25, 86, modeBadge, 0.4f, 0.95f, 0.5f, false);
    }
    drawText(25, 102, LOC("[ESC] 暂停/设置 | [F5] 保存 | [F9] 读取 | [H] 帮助", "[ESC] Pause/Settings | [F5] Save | [F9] Load | [H] Help"), 0.65f, 0.75f, 0.85f, false);'''

assert old_depth in c, "old_depth not found"
c = c.replace(old_depth, new_depth, 1)

# 6. Novice Tutorial Quest Box
old_quest = '''    drawText(qBoxX + 15, qBoxY + 22, "QUEST GUIDE (新手探险指南)", 1.0f, 0.85f, 0.2f, true);

    if (g_TutorialStep == TUTORIAL_MOVE_JUMP) {
        drawText(qBoxX + 15, qBoxY + 44, "Step 1: Basic Movement & Jump", 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Use [A / D] to move, [Space] to Jump", 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "[%s] Move Left  [%s] Move Right  [%s] Jump",
            g_TutMovedLeft ? "V" : " ", g_TutMovedRight ? "V" : " ", g_TutJumped ? "V" : " ");
        drawText(qBoxX + 15, qBoxY + 82, prog, 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_MINE) {
        drawText(qBoxX + 15, qBoxY + 44, "Step 2: Resource Gathering", 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Hold [Left Click] to mine 5 blocks", 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "Progress: %d / 5 blocks mined", g_MinedCount);
        drawText(qBoxX + 15, qBoxY + 82, prog, 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_BUILD) {
        drawText(qBoxX + 15, qBoxY + 44, "Step 3: Block Construction", 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Select block [1-9], [Right Click] to place", 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, "Target must be adjacent to solid block", 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_TORCH) {
        drawText(qBoxX + 15, qBoxY + 44, "Step 4: Underground Illumination", 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Select Torch [5] & place to light dark", 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, "Torches emit warm dynamic light", 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_EXPLORE) {
        drawText(qBoxX + 15, qBoxY + 44, "Step 5: Quick Save & Explore", 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Press [F5] to Quick Save your game!", 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, "Press [H] anytime for full Keybinds", 0.3f, 0.95f, 0.4f, false);
    } else {
        drawText(qBoxX + 15, qBoxY + 44, "Tutorial Complete! Master Explorer", 0.3f, 0.95f, 0.3f, false);
        drawText(qBoxX + 15, qBoxY + 62, "Explore Desert, Tundra & Underworld!", 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, "[F5] Quick Save  [F9] Quick Load  [H] Help", 1.0f, 0.85f, 0.2f, false);
    }'''

new_quest = '''    drawText(qBoxX + 15, qBoxY + 22, LOC("新手探险指南", "Quest Guide"), 1.0f, 0.85f, 0.2f, true);

    if (g_TutorialStep == TUTORIAL_MOVE_JUMP) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 1: 基础移动与跳跃", "Step 1: Movement & Jump"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("使用 [A / D] 移动，[Space] 跳跃", "Use [A / D] to move, [Space] to Jump"), 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "[%s] %s  [%s] %s  [%s] %s",
            g_TutMovedLeft ? "V" : " ", LOC("左移", "Left"),
            g_TutMovedRight ? "V" : " ", LOC("右移", "Right"),
            g_TutJumped ? "V" : " ", LOC("跳跃", "Jump"));
        drawText(qBoxX + 15, qBoxY + 82, prog, 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_MINE) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 2: 资源采集", "Step 2: Resource Gathering"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("按住 [鼠标左键] 挖掘 5 个方块", "Hold [Left Click] to mine 5 blocks"), 0.75f, 0.85f, 0.95f, false);
        char prog[64];
        snprintf(prog, sizeof(prog), "%s: %d / 5", LOC("采集进度", "Progress"), g_MinedCount);
        drawText(qBoxX + 15, qBoxY + 82, prog, 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_BUILD) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 3: 方块搭建", "Step 3: Block Construction"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("选择方块 [1-9]，[鼠标右键] 放置", "Select block [1-9], [Right Click] to place"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("目标位置须与已有方块相邻", "Target must be adjacent to solid block"), 0.3f, 0.95f, 0.4f, false);
    } else if (g_TutorialStep == TUTORIAL_TORCH) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 4: 地下照明", "Step 4: Underground Illumination"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("选择火把 [5] 并放置以照亮黑暗", "Select Torch [5] & place to light dark"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("火把可散发暖色动态照明", "Torches emit warm dynamic light"), 1.0f, 0.85f, 0.2f, false);
    } else if (g_TutorialStep == TUTORIAL_EXPLORE) {
        drawText(qBoxX + 15, qBoxY + 44, LOC("步骤 5: 快速存档与探索", "Step 5: Quick Save & Explore"), 1.0f, 1.0f, 1.0f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("按 [F5] 快速保存当前世界进度！", "Press [F5] to Quick Save your game!"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("随时按 [H] 可查看全键位说明", "Press [H] anytime for Keybinds"), 0.3f, 0.95f, 0.4f, false);
    } else {
        drawText(qBoxX + 15, qBoxY + 44, LOC("新手引导完成！探索大师", "Tutorial Complete! Master Explorer"), 0.3f, 0.95f, 0.3f, false);
        drawText(qBoxX + 15, qBoxY + 62, LOC("自由探索沙漠、雪原与地下熔岩！", "Explore Desert, Tundra & Underworld!"), 0.75f, 0.85f, 0.95f, false);
        drawText(qBoxX + 15, qBoxY + 82, LOC("[F5] 保存  [F9] 读取  [H] 帮助", "[F5] Quick Save  [F9] Quick Load  [H] Help"), 1.0f, 0.85f, 0.2f, false);
    }'''

assert old_quest in c, "old_quest not found"
c = c.replace(old_quest, new_quest, 1)

# 7. Hovered Block Inspector Tooltip
old_tip = '''        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s (%s)", def->name, def->chineseName);
        drawText(tipX + 12, tipY + 22, nameBuf, 1.0f, 1.0f, 1.0f, true);

        char statBuf[128];
        snprintf(statBuf, sizeof(statBuf), "Hardness: %.1f  Drop: %s  [In Reach: YES]",
            def->hardness, def->dropName);
        drawText(tipX + 12, tipY + 42, statBuf, 0.3f, 0.95f, 0.4f, false);'''

new_tip = '''        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s",
            (getLanguage() == LANG_ZH) ? def->chineseName : def->name);
        drawText(tipX + 12, tipY + 22, nameBuf, 1.0f, 1.0f, 1.0f, true);

        char statBuf[128];
        const char* dropStr = (getLanguage() == LANG_ZH) ? def->chineseName : def->dropName;
        snprintf(statBuf, sizeof(statBuf), "%s: %.1f  %s: %s  [%s: %s]",
            LOC("硬度", "Hardness"), def->hardness,
            LOC("掉落", "Drop"), dropStr,
            LOC("触及范围", "In Reach"), LOC("是", "YES"));
        drawText(tipX + 12, tipY + 42, statBuf, 0.3f, 0.95f, 0.4f, false);'''

assert old_tip in c, "old_tip not found"
c = c.replace(old_tip, new_tip, 1)

# 8. Hotbar Item banner & empty slot
old_hotbar_banner = '''        if (isToolItem(actType)) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s (%s)",
                g_ActiveSlot + 1, getItemName(actType), getItemChineseName(actType));
        } else if (actType < BLOCK_TYPE_COUNT) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s (%s)  x%d",
                g_ActiveSlot + 1, g_BlockDefs[actType].name, g_BlockDefs[actType].chineseName, actCount);
        }
    } else {
        snprintf(actBanner, sizeof(actBanner), "[%d] (Empty)", g_ActiveSlot + 1);
    }'''

new_hotbar_banner = '''        if (isToolItem(actType)) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s",
                g_ActiveSlot + 1, (getLanguage() == LANG_ZH) ? getItemChineseName(actType) : getItemName(actType));
        } else if (actType < BLOCK_TYPE_COUNT) {
            snprintf(actBanner, sizeof(actBanner), "[%d] %s  x%d",
                g_ActiveSlot + 1, (getLanguage() == LANG_ZH) ? g_BlockDefs[actType].chineseName : g_BlockDefs[actType].name, actCount);
        }
    } else {
        snprintf(actBanner, sizeof(actBanner), "[%d] (%s)", g_ActiveSlot + 1, LOC("空", "Empty"));
    }'''

assert old_hotbar_banner in c, "old_hotbar_banner not found"
c = c.replace(old_hotbar_banner, new_hotbar_banner, 1)

# 9. Creative Flight banner in HUD
old_flight_banner = '''        if (isCreativeFlying()) {
            drawText(flyX + 15, flyY + 16, "[FLYING ACTIVATED] Space: Up | Shift/S: Down | 2x Space: Land", 0.35f, 0.95f, 1.0f, true);
        } else {
            drawText(flyX + 18, flyY + 16, "[CREATIVE MODE] Double-tap Space to Fly | Infinite Blocks", 1.0f, 0.85f, 0.35f, false);
        }'''

new_flight_banner = '''        if (isCreativeFlying()) {
            drawText(flyX + 15, flyY + 16, LOC("[创造飞行中] 空格: 上升 | Shift/S: 下降 | 双击空格: 着陆", "[FLYING ACTIVE] Space: Up | Shift/S: Down | 2x Space: Land"), 0.35f, 0.95f, 1.0f, true);
        } else {
            drawText(flyX + 18, flyY + 16, LOC("[创造模式] 双击空格开启飞行 | 无限方块放置", "[CREATIVE MODE] Double-tap Space to Fly | Infinite Blocks"), 1.0f, 0.85f, 0.35f, false);
        }'''

assert old_flight_banner in c, "old_flight_banner not found"
c = c.replace(old_flight_banner, new_flight_banner, 1)

# Mining progress text
c = c.replace('drawText(bx + 60, by + 11, "MINING...", 0.1f, 0.1f, 0.1f, false);',
              'drawText(bx + 60, by + 11, LOC("挖掘中...", "MINING..."), 0.1f, 0.1f, 0.1f, false);')

# 10. Help Menu Overlay
old_help = '''    // 8. Help Overlay
    if (g_ShowHelp) {
        float hw = 480.0f;
        float hh = 310.0f;
        float hx = ((float)WINDOW_WIDTH - hw) * 0.5f;
        float hy = ((float)WINDOW_HEIGHT - hh) * 0.5f;
        drawQuad2D(hx, hy, hw, hh, 0.06f, 0.08f, 0.12f, 0.96f);
        drawBorder2D(hx, hy, hw, hh, 0.35f, 0.75f, 0.95f, 2.0f);

        drawText(hx + 20, hy + 30, "KEYBINDS & CONTROLS (全操作指南)", 0.35f, 0.85f, 1.0f, true);
        drawText(hx + 20, hy + 65, "[A / D] or [Left/Right] : Move (1-block auto climb)", 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 90, "[Space] or [W]          : Variable Jump", 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 115, "[Left Mouse Button]     : Mine / Destroy Block", 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 140, "[Right Mouse Button]    : Place Selected Block", 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 165, "[1 - 9] or Mouse Wheel  : Switch Active Hotbar Slot", 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 190, "[F5]                    : Quick Save Game (快速保存)", 0.3f, 0.95f, 0.4f, false);
        drawText(hx + 20, hy + 215, "[F9]                    : Quick Load Game (快速读取)", 0.3f, 0.85f, 1.0f, false);
        drawText(hx + 20, hy + 230, "[C]                     : Toggle Crafting Station (打开工作台)", 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 246, "[E]                     : Inventory & Crafting (背包与合成)", 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 262, "[R]                     : Quick Respawn to Surface (重生)", 1.0f, 0.85f, 0.2f, false);
        drawText(hx + 20, hy + 265, "[H] or [F1]             : Toggle This Help Menu", 0.8f, 0.8f, 0.8f, false);
        drawText(hx + 20, hy + 290, "Auto-saves on exit. Standalone 2.5D OpenGL Engine.", 0.6f, 0.6f, 0.6f, false);
    }'''

new_help = '''    // 8. Help Overlay
    if (g_ShowHelp) {
        float hw = 520.0f;
        float hh = 370.0f;
        float hx = ((float)WINDOW_WIDTH - hw) * 0.5f;
        float hy = ((float)WINDOW_HEIGHT - hh) * 0.5f;
        drawQuad2D(hx, hy, hw, hh, 0.06f, 0.08f, 0.12f, 0.96f);
        drawBorder2D(hx, hy, hw, hh, 0.35f, 0.75f, 0.95f, 2.0f);

        drawText(hx + 20, hy + 28, LOC("操作指南与快捷键", "KEYBINDS & CONTROLS"), 0.35f, 0.85f, 1.0f, true);
        drawText(hx + 20, hy + 58, LOC("[A / D] 或 [左右方向键]  : 左右移动 (支持1格高度自动攀爬)", "[A / D] or [Left/Right] : Move (1-block auto climb)"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 82, LOC("[空格] 或 [W]           : 跳跃 (长按可跳跃更高)", "[Space] or [W]          : Variable Jump"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 106, LOC("[鼠标左键]              : 挖掘 / 破坏方块 / 攻击生物", "[Left Click]            : Mine / Destroy Block / Attack"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 130, LOC("[鼠标右键]              : 放置当前选中的方块", "[Right Click]           : Place Selected Block"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 154, LOC("[1 - 9] 或 鼠标滚轮     : 切换快捷栏栏位", "[1 - 9] or Mouse Wheel  : Switch Active Hotbar Slot"), 1.0f, 1.0f, 1.0f, false);
        drawText(hx + 20, hy + 178, LOC("[F5]                    : 快速保存世界存档", "[F5]                    : Quick Save Game"), 0.3f, 0.95f, 0.4f, false);
        drawText(hx + 20, hy + 202, LOC("[F9]                    : 快速读取世界存档", "[F9]                    : Quick Load Game"), 0.3f, 0.85f, 1.0f, false);
        drawText(hx + 20, hy + 226, LOC("[C]                     : 打开 / 关闭合成工作台", "[C]                     : Toggle Crafting Station"), 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 250, LOC("[E]                     : 打开 / 关闭创造模式物品库", "[E]                     : Toggle Creative Inventory"), 1.0f, 0.90f, 0.35f, false);
        drawText(hx + 20, hy + 274, LOC("[ESC]                   : 暂停菜单 / 切换模式 / 游戏设置", "[ESC]                   : Pause Menu / Mode / Settings"), 1.0f, 0.85f, 0.30f, false);
        drawText(hx + 20, hy + 298, LOC("[R]                     : 立即重生并返回地表安全点", "[R]                     : Quick Respawn to Surface"), 1.0f, 0.85f, 0.2f, false);
        drawText(hx + 20, hy + 322, LOC("[H] 或 [F1]             : 打开 / 关闭本帮助菜单", "[H] or [F1]             : Toggle This Help Menu"), 0.8f, 0.8f, 0.8f, false);
        drawText(hx + 20, hy + 348, LOC("游戏退出时自动保存。独立 2.5D OpenGL 引擎。", "Auto-saves on exit. Standalone 2.5D OpenGL Engine."), 0.6f, 0.6f, 0.6f, false);
    }'''

assert old_help in c, "old_help not found"
c = c.replace(old_help, new_help, 1)

# 11. State rendering dispatch in main loop
old_state_dispatch = '''        } else if (getGameState() == STATE_MENU) {
            renderMainMenu();
        } else if (getGameState() == STATE_PAUSED) {
            renderHUD();
            renderPauseMenu();
        }'''

new_state_dispatch = '''        } else if (getGameState() == STATE_MENU) {
            renderMainMenu();
        } else if (getGameState() == STATE_PAUSED) {
            renderHUD();
            renderPauseMenu();
        } else if (getGameState() == STATE_SETTINGS) {
            if (getPreviousGameState() == STATE_PAUSED) {
                renderHUD();
            }
            renderSettingsDialog();
        }'''

assert old_state_dispatch in c, "old_state_dispatch not found"
c = c.replace(old_state_dispatch, new_state_dispatch, 1)

# 12. Startup loadSettings
old_init = '''    initGame();
    initMenuAndCreative();'''

new_init = '''    loadSettings();
    initGame();
    initMenuAndCreative();'''

assert old_init in c, "old_init not found"
c = c.replace(old_init, new_init, 1)

# Convert back to CRLF
c_crlf = c.replace('\n', '\r\n')

with open(main_path, 'w', encoding='utf-8', newline='') as f:
    f.write(c_crlf)

print("[SUCCESS] standalone_src/main.c refactored successfully!")
