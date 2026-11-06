import subprocess
import time
import ctypes
from ctypes import wintypes
import sys
import os

user32 = ctypes.windll.user32
kernel32 = ctypes.windll.kernel32

WM_KEYDOWN = 0x0100
WM_KEYUP = 0x0101
WM_CHAR = 0x0102
WM_MOUSEMOVE = 0x0200
WM_LBUTTONDOWN = 0x0201
WM_LBUTTONUP = 0x0202
WM_RBUTTONDOWN = 0x0204
WM_RBUTTONUP = 0x0205
WM_CLOSE = 0x0010

VK_SPACE = 0x20
VK_ESCAPE = 0x1B
VK_RETURN = 0x0D
VK_F1 = 0x70
VK_F5 = 0x74
VK_F9 = 0x78

def make_lparam(x, y):
    return (int(y) << 16) | (int(x) & 0xFFFF)

def find_game_window(title_substring="Luminora"):
    found_hwnd = None
    def enum_windows_callback(hwnd, extra):
        nonlocal found_hwnd
        if user32.IsWindowVisible(hwnd):
            length = user32.GetWindowTextLengthW(hwnd)
            if length > 0:
                buff = ctypes.create_unicode_buffer(length + 1)
                user32.GetWindowTextW(hwnd, buff, length + 1)
                if title_substring in buff.value:
                    found_hwnd = hwnd
                    return False
        return True
    
    WNDENUMPROC = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
    user32.EnumWindows(WNDENUMPROC(enum_windows_callback), 0)
    return found_hwnd

def run_playtest():
    print("[1/5] Launching Luminora_Standalone.exe in live mode...")
    exe_path = os.path.abspath("Luminora_Standalone.exe")
    if not os.path.exists(exe_path):
        print(f"ERROR: {exe_path} not found!")
        return False
    
    proc = subprocess.Popen([exe_path], cwd=os.path.dirname(exe_path))
    print(f"      Process spawned, PID: {proc.pid}")
    
    # Wait for window to initialize
    hwnd = None
    for _ in range(30):
        time.sleep(0.2)
        hwnd = find_game_window()
        if hwnd:
            break
        if proc.poll() is not None:
            print(f"ERROR: Process exited early with code {proc.returncode}")
            return False

    if not hwnd:
        print("ERROR: Window not found within 6 seconds!")
        proc.kill()
        return False

    rect = wintypes.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    cw = rect.right - rect.left
    ch = rect.bottom - rect.top
    print(f"      Window found: HWND={hex(hwnd)}, ClientSize={cw}x{ch}")
    
    def post_click(x, y):
        lp = make_lparam(x, y)
        user32.PostMessageW(hwnd, WM_MOUSEMOVE, 0, lp)
        time.sleep(0.04)
        user32.PostMessageW(hwnd, WM_LBUTTONDOWN, 1, lp)
        time.sleep(0.04)
        user32.PostMessageW(hwnd, WM_LBUTTONUP, 0, lp)
        time.sleep(0.06)

    def post_key(vk, ch=None):
        user32.PostMessageW(hwnd, WM_KEYDOWN, vk, 0)
        if ch:
            user32.PostMessageW(hwnd, WM_CHAR, ord(ch), 0)
        time.sleep(0.04)
        user32.PostMessageW(hwnd, WM_KEYUP, vk, 0)
        time.sleep(0.06)

    print("\n[2/5] Testing Main Menu & World Selection Screen Navigation...")
    # Step 1: Main Menu Button 0: Survival Mode [x=450, y=185, w=380, h=46] -> Center: (640, 208)
    print("      -> Clicking 'Survival Mode' button (640, 208)...")
    post_click(640, 208)
    time.sleep(0.3)
    if proc.poll() is not None:
        print("CRASH during main menu navigation!")
        return False

    # Step 2: In STATE_WORLDS_LIST: Slot 0 Create Button [slotX=310, slotW=660, createX=805, createY=173, w=150, h=42]
    # Center: (880, 194)
    print("      -> In Worlds List: Clicking Slot 0 'Create World' button (880, 194)...")
    post_click(880, 194)
    time.sleep(0.3)
    if proc.poll() is not None:
        print("CRASH during world list click!")
        return False

    # Step 3: In STATE_CREATE_WORLD: Randomize Seed Button (Center: 640, 403)
    print("      -> In Create World Modal: Clicking Randomize Seed button (640, 403)...")
    post_click(640, 403)
    time.sleep(0.2)
    # Cycle Name button (Center: 640, 243)
    print("      -> In Create World Modal: Cycling Name button (640, 243)...")
    post_click(640, 243)
    time.sleep(0.2)
    # Click Create & Enter World button: [cBtnX=385, cBtnY=515, w=240, h=46] -> Center: (505, 538)
    print("      -> In Create World Modal: Clicking 'Create & Enter World' (505, 538)...")
    post_click(505, 538)
    time.sleep(0.8)
    if proc.poll() is not None:
        print("CRASH during world generation and start!")
        return False

    print("\n[3/5] Testing In-Game Interactions & Systems...")
    # Step 4: Player movement & Jump
    print("      -> Simulating Player walk Left (A), walk Right (D), and Variable Jump (Space)...")
    user32.PostMessageW(hwnd, WM_KEYDOWN, ord('D'), 0)
    time.sleep(0.4)
    user32.PostMessageW(hwnd, WM_KEYUP, ord('D'), 0)
    time.sleep(0.1)

    user32.PostMessageW(hwnd, WM_KEYDOWN, VK_SPACE, 0)
    time.sleep(0.3)
    user32.PostMessageW(hwnd, WM_KEYUP, VK_SPACE, 0)
    time.sleep(0.2)

    # Step 5: Test Creative Inventory Modal [E]
    print("      -> Pressing [E] to open Creative Inventory...")
    post_key(ord('E'), 'e')
    time.sleep(0.3)
    # Hover over item slot to trigger tooltip
    user32.PostMessageW(hwnd, WM_MOUSEMOVE, 0, make_lparam(300, 200))
    time.sleep(0.2)
    # Close Creative Inventory with [E]
    print("      -> Closing Creative Inventory [E]...")
    post_key(ord('E'), 'e')
    time.sleep(0.3)

    # Step 6: Test Crafting Station Modal [C]
    print("      -> Pressing [C] to open Crafting Station...")
    post_key(ord('C'), 'c')
    time.sleep(0.3)
    # Switch tab to Inventory Preview [tab2 center: 640 + 145/2 = 712, 96 + 15 = 111]
    # modalX = 220, tabX = modalX + 460 = 680, tabW = 145, tab2X = 680 + 145 + 8 = 833, tabY = 80 + 16 = 96
    print("      -> Clicking 'Inventory Preview' tab in Crafting Station (890, 111)...")
    post_click(890, 111)
    time.sleep(0.3)
    # Click Return to Crafting tab
    print("      -> Clicking 'Crafting' tab in Crafting Station (750, 111)...")
    post_click(750, 111)
    time.sleep(0.3)
    # Close Crafting Station with [C]
    print("      -> Closing Crafting Station [C]...")
    post_key(ord('C'), 'c')
    time.sleep(0.3)

    print("\n[4/5] Testing Pause Menu, Settings, Language Toggle & Save/Load...")
    # Step 7: Press ESC for Pause Menu
    print("      -> Pressing [ESC] to enter Pause Menu...")
    post_key(VK_ESCAPE)
    time.sleep(0.3)
    
    # Click Settings button in Pause Menu: boxX=420, boxY=115, btnStartY=115+76=191, btn 3 (Settings) y=191+3*46=329, w=360, h=38
    # Center: (640, 348)
    print("      -> Clicking 'Settings' in Pause Menu (640, 348)...")
    post_click(640, 348)
    time.sleep(0.3)

    # Settings dialog: Language toggle button 0 (boxX=410, boxY=180, btnStartY=180+85=265, center: 640, 287)
    print("      -> Toggling Language in Settings (640, 287) [Switch to English]...")
    post_click(640, 287)
    time.sleep(0.3)
    print("      -> Toggling Language back to Chinese (640, 287)...")
    post_click(640, 287)
    time.sleep(0.3)
    # Click Done & Return button 3: y=265+3*56=433, center: (640, 455)
    print("      -> Clicking 'Done & Return' (640, 455)...")
    post_click(640, 455)
    time.sleep(0.3)

    # Back in Pause menu: Click Resume Game (button 0: 640, 210)
    print("      -> Clicking 'Resume Game' (640, 210)...")
    post_click(640, 210)
    time.sleep(0.3)

    # Step 8: Test Quick Save [F5] & Quick Load [F9]
    print("      -> Pressing [F5] for Quick Save...")
    post_key(VK_F5)
    time.sleep(0.4)
    print("      -> Pressing [F9] for Quick Load...")
    post_key(VK_F9)
    time.sleep(0.4)

    # Step 9: Test Respawn [R]
    print("      -> Pressing [R] for Surface Respawn...")
    post_key(ord('R'), 'r')
    time.sleep(0.4)

    # Step 10: Toggle Help Menu [H]
    print("      -> Pressing [H] to Toggle Help Keybinds Overlay...")
    post_key(ord('H'), 'h')
    time.sleep(0.3)
    post_key(ord('H'), 'h')
    time.sleep(0.3)

    print("\n[5/5] Graceful Shutdown Verification...")
    if proc.poll() is not None:
        print("ERROR: Process terminated prematurely!")
        return False

    print("      -> Sending WM_CLOSE for graceful shutdown & auto-save...")
    user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
    
    exit_code = None
    for _ in range(25):
        time.sleep(0.2)
        exit_code = proc.poll()
        if exit_code is not None:
            break

    if exit_code is None:
        print("WARNING: Process did not exit within 5s, terminating...")
        proc.kill()
        return False
    elif exit_code == 0:
        print(f"SUCCESS: Process exited cleanly with code {exit_code}!")
        return True
    else:
        print(f"WARNING: Process exited with non-zero exit code: {exit_code}")
        return False

if __name__ == '__main__':
    ok = run_playtest()
    sys.exit(0 if ok else 1)
