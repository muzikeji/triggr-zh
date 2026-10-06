// The single catalogue of modes, triggers and actions, shared by Settings and
// the tweak so the two can never disagree about ids.
//
// Each assignment is its own key in the prefs domain:
//   "<mode>/<trigger id>" = "<action id>"   ("" = none)

#define TGDomain @"com.johndie.triggr"
#define TGPrefsChangedNotification "com.johndie.triggr/prefs-changed"
#define TGEnabledKey @"Enabled" // master switch, default on
#define TGRequireUnlockKey @"RequireUnlock" // Lock Screen: shell commands wait for unlock, default on
#define TGBlockedAppsKey @"BlockedApps"     // bundle ids where triggers are ignored
#define TGShowBannersKey @"ShowBanners"     // banner naming what ran, default off
#define TGMenusKey @"Menus"                 // [{id, name}]; items stored at "menu/<id>"
#define TGProfilesKey @"Profiles"           // {name: setup}
#define TGActiveProfileKey @"ActiveProfile"
#define TGAllowAPIKey @"AllowAPI"           // other tweaks / the triggr tool may run actions, default off
#define TGLockReplacesKey @"LockReplaces"   // assigned button presses replace iOS's own action (all buttons), default on
// API: notify_post(TGAPIPrefix "run/<action>" | "trigger/<trigger>" | "menu/<id>").
#define TGAPIPrefix "com.johndie.triggr/api/"

typedef struct { const char *identifier; const char *title; } TGItem;
typedef struct { const char *title; const TGItem *items; int count; const char *footer; } TGGroup;

#define TG_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

// Where an assignment applies, as in Activator.
static const TGItem TGModes[] = {
    {"anywhere", "任意位置"},
    {"home",     "主屏幕"},
    {"app",      "应用内"},
    {"lock",     "锁屏"},
};

static const TGItem TGHomeButton[] = {
    {"home.single", "单击"}, {"home.double", "双击"}, {"home.triple", "三击"},
    {"home.shorthold", "短按"}, {"home.longhold", "长按"},
};
static const TGItem TGTouchID[] = {
    {"touchid.doubletap", "轻触两下"}, {"touchid.rest", "手指停留（锁屏）"}, {"touchid.match", "指纹匹配（锁屏）"},
};
static const TGItem TGLockButton[] = {
    {"lock.single", "单击"}, {"lock.double", "双击"}, {"lock.triple", "三击"}, {"lock.longhold", "长按"},
};
static const TGItem TGVolume[] = {
    {"volume.up", "按音量加"}, {"volume.down", "按音量减"}, {"volume.uphold", "长按音量加"},
    {"volume.downhold", "长按音量减"}, {"volume.updown", "先加后减"}, {"volume.downup", "先减后加"},
    {"volume.both", "同时按下"}, {"volume.bothhold", "同时长按"},
};
static const TGItem TGMuteSwitch[] = { {"mute.silent", "拨到静音"}, {"mute.ring", "拨到响铃"}, {"mute.toggle", "切换"} };
static const TGItem TGStatusBar[] = { {"statusbar.tap", "单击"}, {"statusbar.doubletap", "双击"}, {"statusbar.hold", "长按"} };
// Shake rides on iOS's own shake detection, so no sensor runs for Triggr.
static const TGItem TGMotion[] = { {"motion.shake", "摇动设备"} };
// A quick flick that starts on a Home Screen icon (see TGFlickRecognizer).
static const TGItem TGIcons[] = {
    {"icon.flickup", "上滑"}, {"icon.flickdown", "下滑"}, {"icon.flickleft", "左滑"}, {"icon.flickright", "右滑"},
};
// Not offered: slide-in edge gestures and Home Screen pinches (they'd compete
// with system gestures), status bar swipes (apps draw their own status bar).
static const TGItem TGOther[] = {
    {"power.connected", "连接充电器"}, {"power.disconnected", "断开充电器"},
    {"headphones.in", "连接耳机"}, {"headphones.out", "断开耳机"},
};

static const TGItem TGStateChanges[] = {
    {"wifi.on", "Wi-Fi 开启"}, {"wifi.off", "Wi-Fi 关闭"},
    {"wifi.joined", "连接 Wi-Fi 网络"}, {"wifi.left", "断开 Wi-Fi 网络"},
    {"bluetooth.on", "蓝牙开启"}, {"bluetooth.off", "蓝牙关闭"},
    {"lowpower.on", "低电量模式开启"}, {"lowpower.off", "低电量模式关闭"},
    {"device.locked", "设备已锁定"}, {"device.unlocked", "设备已解锁"},
    {"display.on", "屏幕开启"}, {"display.off", "屏幕关闭"},
};

static const TGGroup TGTriggerGroups[] = {
    {"主屏幕按钮", TGHomeButton, TG_COUNT(TGHomeButton), NULL},
    {"触控 ID", TGTouchID, TG_COUNT(TGTouchID), "轻触两下指不按下按钮而触碰两次传感器。解锁状态下 iOS 不会报告单击或长按。手指停留与指纹匹配仅在锁屏生效，并与解锁同时运行。"},
    {"锁屏按钮", TGLockButton, TG_COUNT(TGLockButton), NULL},
    {"音量按钮", TGVolume, TG_COUNT(TGVolume), "先加后减（及反向）是两次快速按键；它们始终同时运行，音量最终回到起始值。"},
    {"静音开关", TGMuteSwitch, TG_COUNT(TGMuteSwitch), "开启「替换按钮操作」时，开关位置与响铃状态可能不一致，直到你拨回原位。"},
    {"状态栏", TGStatusBar, TG_COUNT(TGStatusBar), "在主屏幕、锁屏和应用内均可使用。单击仍会滚动到顶部。长按为半秒。"},
    {"主屏幕图标", TGIcons, TG_COUNT(TGIcons), "从主屏幕或程序坞的应用或文件夹图标开始的快速滑动。小组件、App 资源库和抖动模式不受影响。左滑和右滑会接管从图标开始的翻页滑动。"},
    {"动作", TGMotion, TG_COUNT(TGMotion), "使用 iOS 自带的摇动检测（即「摇动以撤销」所用），不额外耗电。手机解锁并亮屏时可用；应用提供时「摇动以撤销」仍会出现。"},
    {"充电器与耳机", TGOther, TG_COUNT(TGOther), "取出 AirPod 可能被计为「断开耳机」，因为 iOS 会把声音切换到扬声器。"},
    {"状态变化", TGStateChanges, TG_COUNT(TGStateChanges), "在变化发生后运行。控制中心的 Wi-Fi 按钮只会断开网络（「断开 Wi-Fi 网络」），Wi-Fi 仍保持开启。由 Triggr 自身操作引起的变化会被忽略一秒，因此分配不会循环触发。"},
};

static const TGItem TGSystemActions[] = {
    {"system.home", "回到主屏幕"}, {"system.switcher", "应用切换器"},
    {"system.lastapp", "上一个应用"}, {"system.quitapp", "退出当前应用"},
    {"system.cc", "控制中心"}, {"system.nc", "通知中心"}, {"system.spotlight", "聚焦搜索"},
    {"system.reachability", "便捷访问"}, {"system.siri", "Siri"}, {"system.screenshot", "截屏"},
    {"system.screenrecord", "屏幕录制"}, {"system.closeapps", "关闭后台应用"},
    {"system.vibrate", "振动"}, {"system.nothing", "无操作"},
};
static const TGItem TGPowerActions[] = {
    {"system.sleep", "休眠"}, {"system.lock", "锁定设备"}, {"system.respring", "重启桌面"}, {"system.powerdown", "关机滑块"},
    {"system.safemode", "安全模式"}, {"system.restart", "重启"}, {"system.poweroff", "关机"},
};
// Switches: toggle.<name>, on.<name>, off.<name> (like Activator's Flipswitch actions).
// Siri: no safe SpringBoard entry point found yet.
#define TG_SWITCHES(X) \
    X("flashlight", "手电筒") X("wifi", "Wi-Fi") X("bluetooth", "蓝牙") X("airplane", "飞行模式") \
    X("cellular", "蜂窝数据") X("dnd", "勿扰模式") X("lowpower", "低电量模式") X("rotation", "旋转锁定") \
    X("mute", "静音") X("darkmode", "深色模式") X("nightshift", "夜览") X("autobrightness", "自动亮度") \
    X("keepawake", "屏幕常亮") X("location", "定位服务")
#define TG_SWITCH_NAME(id, name) {id, name},
#define TG_SWITCH_TOGGLE(id, name) {"toggle." id, "切换" name},
#define TG_SWITCH_ON(id, name) {"on." id, name "开启"},
#define TG_SWITCH_OFF(id, name) {"off." id, name "关闭"},
static const TGItem TGSwitches[] = { TG_SWITCHES(TG_SWITCH_NAME) };
static const TGItem TGToggleActions[] = { TG_SWITCHES(TG_SWITCH_TOGGLE) };
static const TGItem TGOnActions[] = { TG_SWITCHES(TG_SWITCH_ON) };
static const TGItem TGOffActions[] = { TG_SWITCHES(TG_SWITCH_OFF) };
static const TGItem TGMediaActions[] = {
    {"media.playpause", "播放 / 暂停"}, {"media.next", "下一首"}, {"media.previous", "上一首"},
    {"media.volup", "音量加"}, {"media.voldown", "音量减"},
    {"media.airplay", "AirPlay 选择"}, {"media.airplayiphone", "在 iPhone 上播放"},
};

static const TGGroup TGActionGroups[] = {
    {"系统", TGSystemActions, TG_COUNT(TGSystemActions), NULL},
    {"电源", TGPowerActions, TG_COUNT(TGPowerActions), NULL},
    {"切换", TGToggleActions, TG_COUNT(TGToggleActions), NULL},
    {"开启", TGOnActions, TG_COUNT(TGOnActions), NULL},
    {"关闭", TGOffActions, TG_COUNT(TGOffActions), NULL},
    {"媒体", TGMediaActions, TG_COUNT(TGMediaActions), NULL},
};

// Command actions carry their argument in the id: "<prefix><text>".
#define TGShortcutPrefix @"shortcut:"
#define TGURLPrefix @"url:"
#define TGShellPrefix @"shell:"
#define TGAppPrefix @"app:"
#define TGPausePrefix @"pause:" // "pause:<seconds>" between actions in a list
#define TGMenuPrefix @"menu:"   // "menu:<id>" shows that menu
#define TGMenuKeyPrefix @"menu/" // "menu/<id>" = the menu's items
// Custom event prefixes (see TGIsCustomTrigger).
#define TGWiFiJoinedPrefix @"wifi.joined:"
#define TGWiFiLeftPrefix @"wifi.left:"
#define TGBTConnectedPrefix @"bt.connected:"
#define TGBTDisconnectedPrefix @"bt.disconnected:"
#define TGBatteryAbovePrefix @"battery.above:"
#define TGBatteryBelowPrefix @"battery.below:"
#define TGAppLaunchedPrefix @"app.launched:"
#define TGTimePrefix @"time:"
// A flick on one app's icon: "icon.flickup:<bundle id>" (runs instead of the plain flick).
#define TGIconFlickUpPrefix @"icon.flickup:"
#define TGIconFlickDownPrefix @"icon.flickdown:"
#define TGIconFlickLeftPrefix @"icon.flickleft:"
#define TGIconFlickRightPrefix @"icon.flickright:"
// Actions with a value.
#define TGBrightnessPrefix @"brightness:"     // 0-100
#define TGMediaVolumePrefix @"volume.media:"  // 0-100
#define TGRingerVolumePrefix @"volume.ringer:" // 0-100
#define TGMessagePrefix @"message:"
#define TGSpeakPrefix @"speak:"
#define TGSettingsPrefix @"settings:" // App-prefs page id
#define TGAirPlayPrefix @"airplay:"   // AirPlay device name (or part of it)
// Settings can't discover AirPlay devices itself: it asks SpringBoard, which
// writes the names it sees to this file and answers with the second notification.
#define TGAirPlayListRequest "com.johndie.triggr/airplay-list"
#define TGAirPlayListReady "com.johndie.triggr/airplay-list-ready"
#define TGAirPlayListPath @"/var/jb/var/mobile/Library/Preferences/com.johndie.triggr.airplay.plist"

// Settings pages for "Open Settings Page" (App-prefs:<id>, verified to open Settings on iOS 16.7).
static const TGItem TGSettingsPages[] = {
    {"WIFI", "Wi-Fi"}, {"Bluetooth", "蓝牙"}, {"MOBILE_DATA_SETTINGS_ID", "蜂窝网络"},
    {"NOTIFICATIONS_ID", "通知"}, {"Sounds", "声音与触感"}, {"General", "通用"},
    {"ControlCenter", "控制中心"}, {"DISPLAY", "显示与亮度"}, {"Wallpaper", "墙纸"},
    {"BATTERY_USAGE", "电池"}, {"Privacy", "隐私与安全性"},
};
#define TGPauseMax 30.0

// Actions that open an app. iOS can't show them over the Lock Screen (verified:
// Open App does nothing there), so on the Lock Screen they wait for unlock.
static inline BOOL TGActionOpensApp(NSString *action) {
    return [action hasPrefix:TGAppPrefix] || [action hasPrefix:TGURLPrefix] || [action hasPrefix:TGShortcutPrefix] || [action hasPrefix:TGSettingsPrefix];
}

// Pauses only make sense between actions: drop leading and trailing ones and
// collapse repeats (used after an action is removed).
static inline NSMutableArray<NSString *> *TGTidyPauses(NSArray<NSString *> *actions) {
    NSMutableArray *result = [NSMutableArray array];
    for (NSString *action in actions) {
        BOOL pause = [action hasPrefix:TGPausePrefix];
        if (pause && (result.count == 0 || [result.lastObject hasPrefix:TGPausePrefix])) continue;
        [result addObject:action];
    }
    while ([result.lastObject hasPrefix:TGPausePrefix]) [result removeLastObject];
    return result;
}

// What a button or the mute switch does by itself, which is what happens while
// nothing is assigned to it (nil: nothing, or not a button trigger).
static inline NSString *TGDefaultActionTitle(NSString *trigger) {
    return @{
        @"home.single": @"回到主屏幕", @"home.double": @"应用切换器", @"home.triple": @"辅助功能快捷键",
        @"home.longhold": @"Siri", @"touchid.doubletap": @"便捷访问",
        @"volume.up": @"音量加", @"volume.down": @"音量减",
        @"lock.single": @"锁定", @"lock.longhold": @"关机滑块",
        @"mute.silent": @"静音", @"mute.ring": @"取消静音", @"mute.toggle": @"静音 / 取消静音",
    }[trigger];
}

// What assigning an action to a trigger does to the button's own action (shown in
// the action picker). Button triggers depend on Replace Button Actions.
static inline NSString *TGTriggerWarning(NSString *trigger, BOOL replaces) {
    NSDictionary *buttons = replaces ? @{
        @"home.single": @"在生效范围内代替回到主屏幕（或唤醒）。若想同时回到主屏幕，请追加「回到主屏幕」。",
        @"home.double": @"代替打开应用切换器。若想同时打开，请追加「应用切换器」。",
        @"touchid.doubletap": @"代替便捷访问（轻触两下）。若想保留，请追加「便捷访问」。",
        @"home.triple": @"代替辅助功能快捷键；双击会稍作等待以判断是否有第三击。",
        @"home.longhold": @"代替 Siri。若想保留 Siri，请追加「Siri」。",
        @"volume.up": @"代替调高音量。若想同时调节，请追加「音量加（媒体）」。",
        @"volume.down": @"代替调低音量。若想同时调节，请追加「音量减（媒体）」。",
        @"volume.both": @"按住一个音量键再按另一个。第二个键不会改变音量；第一个键可能仍会变动一格。",
        @"volume.bothhold": @"同时按住两个音量键 0.5 秒。第二个键不会改变音量；第一个键可能仍会变动一格。",
        @"lock.single": @"在屏幕点亮时代替锁定。若想同时锁定，请追加「休眠」。",
        @"lock.double": @"单击会稍作等待以判断是否有第二击。",
        @"lock.triple": @"单击与双击会稍作等待以判断是否有后续按键。",
        @"lock.longhold": @"代替关机滑块。若想同时显示，请追加「关机滑块」。",
        @"mute.silent": @"代替静音，因此铃声保持开启。若想同时静音，请追加「静音开启」（开关）。",
        @"mute.ring": @"代替取消静音，因此铃声保持关闭。若想同时取消静音，请追加「静音关闭」（开关）。",
        @"mute.toggle": @"代替静音或取消静音。若想同时切换，请追加「切换静音」（开关）。",
    } : @{
        @"home.single": @"与正常按键同时运行，仍会回到主屏幕。",
        @"home.double": @"与「应用切换器」同时运行。",
        @"touchid.doubletap": @"与「便捷访问」同时运行。",
        @"home.triple": @"与「辅助功能快捷键」同时运行。",
        @"home.longhold": @"与 Siri 同时运行。",
        @"volume.up": @"与音量调节同时运行。",
        @"volume.down": @"与音量调节同时运行。",
        @"volume.both": @"按住一个音量键再按另一个。两者仍会改变音量。",
        @"volume.bothhold": @"同时按住两个音量键 0.5 秒。两者仍会改变音量。",
        @"lock.single": @"与每次按键同时运行，仍会锁定。",
        @"lock.double": @"在按键结束后运行；每次按键仍会锁定或唤醒。",
        @"lock.triple": @"在按键结束后运行；每次按键仍会锁定或唤醒。",
        @"lock.longhold": @"与「关机滑块」同时运行。",
        @"mute.silent": @"与开关同时运行，仍会静音。",
        @"mute.ring": @"与开关同时运行，仍会取消静音。",
        @"mute.toggle": @"与开关同时运行，仍会静音和取消静音。",
    };
    if (buttons[trigger]) return replaces ? buttons[trigger] : [buttons[trigger] stringByAppendingString:@" 若要改为代替原操作，请在「选项」中开启「替换按钮操作」。"];
    NSDictionary *warnings = @{
        @"home.shorthold": @"在 Siri 出现前松开的按住。",
        @"volume.uphold": @"按住 0.5 秒后运行；音量仍会变动一格。",
        @"volume.downhold": @"按住 0.5 秒后运行；音量仍会变动一格。",
        @"icon.flickleft": @"从图标开始的翻到下一页的滑动会改为运行此操作。在图标之间滑动可翻页。",
        @"icon.flickright": @"从图标开始的翻到上一页的滑动会改为运行此操作。在图标之间滑动可翻页。",
        @"icon.flickdown": @"从图标开始的向下拉出搜索的滑动会改为运行此操作。",
        @"volume.updown": @"在半秒内先按音量加再按音量减。与这两次按键同时运行。",
        @"volume.downup": @"在半秒内先按音量减再按音量加。与这两次按键同时运行。",
        @"time": @"在 Triggr 运行时于该时间执行。手机休眠时 iOS 可能延迟计时器；若延迟超过 5 分钟则跳过。",
        @"battery": @"当电量越过该百分比时执行一次，而非每次变化都执行。",
        @"statusbar.tap": @"若同时分配了「双击」，单击会稍作等待以判断是否有第二击。",
    };
    if ([trigger hasPrefix:TGTimePrefix]) return warnings[@"time"];
    if ([trigger hasPrefix:@"icon.flick"] && [trigger containsString:@":"]) return warnings[[trigger componentsSeparatedByString:@":"].firstObject];
    if ([trigger hasPrefix:TGBatteryAbovePrefix] || [trigger hasPrefix:TGBatteryBelowPrefix]) return warnings[@"battery"];
    return warnings[trigger];
}

// Display titles for any trigger or mode id.
// Custom events carry a value after the prefix: "wifi.joined:<network>",
// "bt.connected:<device>", "battery.above:<percent>", "app.launched:<bundle id>",
// "time:<HHMM>:<daily|weekdays|weekends>".

static inline NSArray<NSString *> *TGCustomPrefixes(void) {
    return @[TGWiFiJoinedPrefix, TGWiFiLeftPrefix, TGBTConnectedPrefix, TGBTDisconnectedPrefix,
        TGBatteryAbovePrefix, TGBatteryBelowPrefix, TGAppLaunchedPrefix, TGTimePrefix,
        TGIconFlickUpPrefix, TGIconFlickDownPrefix, TGIconFlickLeftPrefix, TGIconFlickRightPrefix];
}

static inline BOOL TGIsCustomTrigger(NSString *trigger) {
    for (NSString *prefix in TGCustomPrefixes())
        if ([trigger hasPrefix:prefix] && trigger.length > prefix.length) return YES;
    return NO;
}

static inline NSString *TGDaysTitle(NSString *days) {
    if ([days isEqualToString:@"weekdays"]) return @"工作日";
    if ([days isEqualToString:@"weekends"]) return @"周末";
    return @"每天";
}

// "time:0730:weekdays" -> hour 7, minute 30, days "weekdays"; NO if malformed.
static inline BOOL TGParseTime(NSString *trigger, int *hour, int *minute, NSString **days) {
    NSArray *parts = [[trigger substringFromIndex:MIN(TGTimePrefix.length, trigger.length)] componentsSeparatedByString:@":"];
    if (parts.count != 2 || [parts[0] length] != 4) return NO;
    int hhmm = [parts[0] intValue];
    *hour = hhmm / 100;
    *minute = hhmm % 100;
    *days = parts[1];
    return *hour < 24 && *minute < 60;
}

// "Flick Up" for "icon.flickup" or "icon.flickup:<bundle id>".
static inline NSString *TGFlickDirectionTitle(NSString *trigger) {
    NSString *direction = [trigger componentsSeparatedByString:@":"].firstObject;
    for (int i = 0; i < TG_COUNT(TGIcons); i++) if ([direction isEqualToString:@(TGIcons[i].identifier)]) return @(TGIcons[i].title);
    return direction;
}

static inline NSString *TGCustomTriggerTitle(NSString *trigger) {
    NSString *value = nil;
    for (NSString *prefix in TGCustomPrefixes()) if ([trigger hasPrefix:prefix]) value = [trigger substringFromIndex:prefix.length];
    if ([trigger hasPrefix:TGWiFiJoinedPrefix]) return [NSString stringWithFormat:@"已连接 “%@”", value];
    if ([trigger hasPrefix:TGWiFiLeftPrefix]) return [NSString stringWithFormat:@"已断开 “%@”", value];
    if ([trigger hasPrefix:TGBTConnectedPrefix]) return [NSString stringWithFormat:@"已连接到 “%@”", value];
    if ([trigger hasPrefix:TGBTDisconnectedPrefix]) return [NSString stringWithFormat:@"已断开与 “%@” 的连接", value];
    if ([trigger hasPrefix:TGBatteryAbovePrefix]) return [NSString stringWithFormat:@"电量升至 %@%% 以上", value];
    if ([trigger hasPrefix:TGBatteryBelowPrefix]) return [NSString stringWithFormat:@"电量降至 %@%% 以下", value];
    if ([trigger hasPrefix:TGAppLaunchedPrefix]) return [@"打开 " stringByAppendingString:value];
    if ([trigger hasPrefix:@"icon.flick"]) return [NSString stringWithFormat:@"%@（%@）", TGFlickDirectionTitle(trigger), value];
    int hour, minute;
    NSString *days;
    if (TGParseTime(trigger, &hour, &minute, &days)) return [NSString stringWithFormat:@"%02d:%02d %@", hour, minute, TGDaysTitle(days)];
    return trigger;
}

static inline NSString *TGTriggerTitle(NSString *trigger) {
    if (TGIsCustomTrigger(trigger)) return TGCustomTriggerTitle(trigger);
    for (int g = 0; g < TG_COUNT(TGTriggerGroups); g++)
        for (int i = 0; i < TGTriggerGroups[g].count; i++)
            if ([trigger isEqualToString:@(TGTriggerGroups[g].items[i].identifier)]) {
                // Events ("Charger Connected") read fine alone; presses need their button.
                BOOL event = TGTriggerGroups[g].items == TGOther || TGTriggerGroups[g].items == TGStateChanges || TGTriggerGroups[g].items == TGMotion;
                if (TGTriggerGroups[g].items == TGIcons) return [NSString stringWithFormat:@"图标%s", TGTriggerGroups[g].items[i].title];
                return event ? @(TGTriggerGroups[g].items[i].title) : [NSString stringWithFormat:@"%s %s", TGTriggerGroups[g].title, TGTriggerGroups[g].items[i].title];
            }
    return trigger;
}

// Home button and Touch ID triggers need a Home button (Touch ID is in it).
static inline BOOL TGTriggerFitsHardware(NSString *trigger, BOOL hasHomeButton) {
    if ([trigger hasPrefix:@"home."] || [trigger hasPrefix:@"touchid."]) return hasHomeButton;
    return YES;
}

// Only catalogue triggers count; keys left over from older versions are ignored.
static inline BOOL TGIsKnownTrigger(NSString *trigger) {
    if (TGIsCustomTrigger(trigger)) return YES;
    for (int g = 0; g < TG_COUNT(TGTriggerGroups); g++)
        for (int i = 0; i < TGTriggerGroups[g].count; i++)
            if ([trigger isEqualToString:@(TGTriggerGroups[g].items[i].identifier)]) return YES;
    return NO;
}

static inline NSString *TGModeTitle(NSString *mode) {
    for (int m = 0; m < TG_COUNT(TGModes); m++) if ([mode isEqualToString:@(TGModes[m].identifier)]) return @(TGModes[m].title);
    return mode;
}

static inline NSString *TGAssignmentKey(NSString *mode, NSString *trigger) {
    return [NSString stringWithFormat:@"%@/%@", mode, trigger];
}

// Display title for an action id ("None" when unassigned).
static inline NSString *TGActionTitle(NSString *action) {
    if (action.length == 0) return @"无";
    if ([action hasPrefix:TGShortcutPrefix]) return [@"快捷指令：" stringByAppendingString:[action substringFromIndex:TGShortcutPrefix.length]];
    if ([action hasPrefix:TGURLPrefix]) return [@"网址：" stringByAppendingString:[action substringFromIndex:TGURLPrefix.length]];
    if ([action hasPrefix:TGShellPrefix]) return [@"命令：" stringByAppendingString:[action substringFromIndex:TGShellPrefix.length]];
    if ([action hasPrefix:TGAppPrefix]) return [@"打开 " stringByAppendingString:[action substringFromIndex:TGAppPrefix.length]];
    if ([action hasPrefix:TGPausePrefix]) return [NSString stringWithFormat:@"暂停 %@ 秒", [action substringFromIndex:TGPausePrefix.length]];
    if ([action hasPrefix:TGMenuPrefix]) return @"菜单";
    if ([action hasPrefix:TGBrightnessPrefix]) return [NSString stringWithFormat:@"亮度 %@%%", [action substringFromIndex:TGBrightnessPrefix.length]];
    if ([action hasPrefix:TGMediaVolumePrefix]) return [NSString stringWithFormat:@"媒体音量 %@%%", [action substringFromIndex:TGMediaVolumePrefix.length]];
    if ([action hasPrefix:TGRingerVolumePrefix]) return [NSString stringWithFormat:@"铃声音量 %@%%", [action substringFromIndex:TGRingerVolumePrefix.length]];
    if ([action hasPrefix:TGMessagePrefix]) return [@"信息：" stringByAppendingString:[action substringFromIndex:TGMessagePrefix.length]];
    if ([action hasPrefix:TGSpeakPrefix]) return [@"朗读：" stringByAppendingString:[action substringFromIndex:TGSpeakPrefix.length]];
    if ([action hasPrefix:TGAirPlayPrefix]) return [@"AirPlay 投放到 " stringByAppendingString:[action substringFromIndex:TGAirPlayPrefix.length]];
    if ([action hasPrefix:TGSettingsPrefix]) {
        NSString *page = [action substringFromIndex:TGSettingsPrefix.length];
        for (int i = 0; i < TG_COUNT(TGSettingsPages); i++)
            if ([page isEqualToString:@(TGSettingsPages[i].identifier)]) return [@"设置：" stringByAppendingString:@(TGSettingsPages[i].title)];
        return [@"设置：" stringByAppendingString:page];
    }
    for (int g = 0; g < TG_COUNT(TGActionGroups); g++)
        for (int i = 0; i < TGActionGroups[g].count; i++)
            if ([action isEqualToString:@(TGActionGroups[g].items[i].identifier)]) return @(TGActionGroups[g].items[i].title);
    return action;
}
