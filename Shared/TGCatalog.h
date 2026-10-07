#import "TGPaths.h"
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
#define TGRecentActionsKey @"RecentActions" // the last actions picked, newest first (Settings only)
// Settings' Test button: SpringBoard runs the assignment stored at TGTestKey ("<mode>/<trigger>") now.
#define TGTestKey @"TestAssignment"
#define TGTestNotification "com.johndie.triggr/test"
// API: notify_post(TGAPIPrefix "run/<action>" | "trigger/<trigger>" | "menu/<id>").
#define TGAPIPrefix "com.johndie.triggr/api/"

typedef struct { const char *identifier; const char *title; } TGItem;
typedef struct { const char *title; const TGItem *items; int count; const char *footer; } TGGroup;

#define TG_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

// Where an assignment applies, as in Activator.
static const TGItem TGModes[] = {
    {"anywhere", "任意位置"},
    {"home",     "主屏幕"},
    {"app",      "在应用内"},
    {"lock",     "锁屏时"},
};

static const TGItem TGHomeButton[] = {
    {"home.single", "单击"}, {"home.double", "双击"}, {"home.triple", "三击"},
    {"home.shorthold", "短按不放"}, {"home.longhold", "长按"},
};
static const TGItem TGTouchID[] = {
    {"touchid.doubletap", "轻点两次"}, {"touchid.rest", "手指停留（锁屏）"}, {"touchid.match", "指纹匹配（锁屏）"},
};
static const TGItem TGLockButton[] = {
    {"lock.single", "单击"}, {"lock.double", "双击"}, {"lock.triple", "三击"}, {"lock.longhold", "长按"},
};
static const TGItem TGVolume[] = {
    {"volume.up", "按音量加"}, {"volume.down", "按音量减"}, {"volume.uphold", "长按音量加"},
    {"volume.downhold", "长按音量减"}, {"volume.updown", "先加后减"}, {"volume.downup", "先减后加"},
    {"volume.both", "同时按下"}, {"volume.bothhold", "久按两个键"},
};
static const TGItem TGMuteSwitch[] = { {"mute.silent", "切换到静音"}, {"mute.ring", "切换到响铃"}, {"mute.toggle", "切换静音/响铃"} };
static const TGItem TGStatusBar[] = {
    {"statusbar.tap", "点按"}, {"statusbar.doubletap", "双击"}, {"statusbar.hold", "长按"},
    {"statusbar.left.tap", "左侧点按"}, {"statusbar.left.doubletap", "左侧双击"}, {"statusbar.left.hold", "左侧长按"},
    {"statusbar.right.tap", "右侧点按"}, {"statusbar.right.doubletap", "右侧双击"}, {"statusbar.right.hold", "右侧长按"},
    {"statusbar.swipeleft", "向左滑动"}, {"statusbar.swiperight", "向右滑动"},
};
// Shake rides on iOS's own shake detection, so no sensor runs for Triggr.
static const TGItem TGMotion[] = { {"motion.shake", "摇一摇"} };
// A quick flick that starts on a Home Screen icon (see TGFlickRecognizer).
// Gestures on the Home Screen pages themselves (not on an icon).
static const TGItem TGHomeGestures[] = {
    {"homescreen.pinchin", "捏合"}, {"homescreen.pinchout", "张开"},
    {"homescreen.twoup", "双指上滑"}, {"homescreen.twodown", "双指下滑"},
    {"homescreen.doubletap", "双击空白处"},
};
static const TGItem TGIcons[] = {
    {"icon.flickup", "上滑"}, {"icon.flickdown", "下滑"}, {"icon.flickleft", "左滑"}, {"icon.flickright", "右滑"},
};
// Not offered: slide-in edge gestures and Home Screen pinches (they'd compete
// with system gestures), status bar swipes (apps draw their own status bar).
static const TGItem TGOther[] = {
    {"power.connected", "充电器已连接"}, {"power.disconnected", "充电器已断开"},
    {"headphones.in", "耳机已连接"}, {"headphones.out", "耳机已断开"},
};

// The proximity sensor (by the earpiece), only kept on while the screen is.
static const TGItem TGProximity[] = { {"proximity.cover", "遮挡"}, {"proximity.wave", "挥手（短暂遮挡）"} };
static const TGItem TGStateChanges[] = {
    {"wifi.on", "Wi-Fi 已开启"}, {"wifi.off", "Wi-Fi 已关闭"},
    {"wifi.joined", "已连接 Wi-Fi 网络"}, {"wifi.left", "已断开 Wi-Fi 网络"},
    {"bluetooth.on", "蓝牙已开启"}, {"bluetooth.off", "蓝牙已关闭"},
    {"lowpower.on", "低电量模式已开启"}, {"lowpower.off", "低电量模式已关闭"},
    {"device.locked", "设备已锁定"}, {"device.unlocked", "设备已解锁"},
    {"display.on", "屏幕已点亮"}, {"display.off", "屏幕已熄灭"},
};

static const TGGroup TGTriggerGroups[] = {
    {"主屏幕按钮", TGHomeButton, TG_COUNT(TGHomeButton), NULL},
    {"触控 ID", TGTouchID, TG_COUNT(TGTouchID), "「轻点两次」是不按下去的状态下轻触传感器两次。解锁状态下 iOS 不报告单击或长按。手指停留和指纹匹配仅在锁屏时生效，并会伴随解锁一起执行。"},
    {"锁屏按钮", TGLockButton, TG_COUNT(TGLockButton), NULL},
    {"音量按键", TGVolume, TG_COUNT(TGVolume), "「先加后减」（以及相反的「先减后加」）是两次快速按动；它们始终伴随音量变动一起执行，音量会回到起点位置。"},
    {"静音开关", TGMuteSwitch, TG_COUNT(TGMuteSwitch), "开启「替换」后，开关的位置与铃声音量可能不一致，直到你把它拨回原位。"},
    {"状态栏", TGStatusBar, TG_COUNT(TGStatusBar), "在主屏幕、锁屏和应用内都可用。单击仍会滑动到顶部。长按为半秒。左侧与右侧分别对应状态栏的一半：当某一侧被分配后，该侧的普通点按、双击或长按将由分配的动作代替。左滑与右滑是沿状态栏的快速横向滑动；向下滑动仍会打开通知中心或控制中心。"},
    {"主屏幕图标", TGIcons, TG_COUNT(TGIcons), "在主屏幕或程序坞的某个应用或文件夹图标上开始的一次快速滑动。小组件、应用资料库和抖动编辑模式都不会触发。左滑与右滑会接管从图标上开始翻页的手势。"},
    {"主屏幕手势", TGHomeGestures, TG_COUNT(TGHomeGestures), "作用于主屏幕页面本身，而非图标。双指滑动与捏合不会干扰单指滚动、搜索（Spotlight）或今日视图；双击仅在空白处生效。在主屏幕编辑状态下或文件夹展开时会忽略。"},
    {"动作", TGMotion, TG_COUNT(TGMotion), "使用 iOS 自身的摇动检测（即「摇动撤销」背后的动作），因此不额外耗电。在手机解锁且唤醒状态下生效；即使应用提供了「摇动撤销」，它仍然照常出现。"},
    {"充电与耳机", TGOther, TG_COUNT(TGOther), "取出 AirPod 可能会被计为「耳机已断开」，因为 iOS 会把声音切回扬声器。"},
    {"距离传感器", TGProximity, TG_COUNT(TGProximity), "听筒旁边的传感器。一旦分配了其中一项，它在屏幕点亮时会保持开启（会略微增加耗电）。「挥手」指遮挡时间不超过半秒；当分配了「挥手」时，「遮挡」会等待这么长时间后才执行。"},
    {"状态变化", TGStateChanges, TG_COUNT(TGStateChanges), "在变化发生后执行。控制中心的 Wi-Fi 按钮只会断开当前网络（触发「已断开 Wi-Fi 网络」），Wi-Fi 本身仍然开启。由 Triggr 自身动作引起的变化会忽略一秒钟，以避免动作循环触发。"},
};

static const TGItem TGSystemActions[] = {
    {"system.home", "前往主屏幕"}, {"system.switcher", "应用切换器"},
    {"system.lastapp", "上一个应用"}, {"system.quitapp", "退出当前应用"},
    {"system.cc", "控制中心"}, {"system.nc", "通知中心"}, {"system.spotlight", "搜索（Spotlight）"},
    {"system.reachability", "便捷访问"}, {"system.siri", "Siri"}, {"system.screenshot", "截屏"},
    {"system.screenrecord", "屏幕录制"}, {"system.closeapps", "关闭后台应用"},
    {"system.vibrate", "震动"}, {"system.nothing", "不做任何事"},
};
static const TGItem TGPowerActions[] = {
    {"system.sleep", "睡眠"}, {"system.lock", "锁定设备"}, {"system.respring", "重启主界面（Respring）"}, {"system.powerdown", "关机滑块"},
    {"system.safemode", "安全模式"}, {"system.restart", "重启"}, {"system.poweroff", "关机"},
};
// Switches: toggle.<name>, on.<name>, off.<name> (like Activator's Flipswitch actions).
// Siri: no safe SpringBoard entry point found yet.
#define TG_SWITCHES(X) \
    X("flashlight", "手电筒") X("wifi", "Wi-Fi") X("bluetooth", "蓝牙") X("airplane", "飞行模式") \
    X("cellular", "蜂窝数据") X("dnd", "勿扰模式") X("lowpower", "低电量模式") X("rotation", "旋转锁定") \
    X("mute", "静音") X("darkmode", "深色模式") X("nightshift", "夜览") X("autobrightness", "自动亮度") \
    X("keepawake", "保持屏幕点亮") X("location", "定位服务")
#define TG_SWITCH_NAME(id, name) {id, name},
#define TG_SWITCH_TOGGLE(id, name) {"toggle." id, "切换" name},
#define TG_SWITCH_ON(id, name) {"on." id, "开启" name},
#define TG_SWITCH_OFF(id, name) {"off." id, "关闭" name},
static const TGItem TGSwitches[] = { TG_SWITCHES(TG_SWITCH_NAME) };
static const TGItem TGToggleActions[] = { TG_SWITCHES(TG_SWITCH_TOGGLE) };
static const TGItem TGOnActions[] = { TG_SWITCHES(TG_SWITCH_ON) };
static const TGItem TGOffActions[] = { TG_SWITCHES(TG_SWITCH_OFF) };
static const TGItem TGMediaActions[] = {
    {"media.playpause", "播放/暂停"}, {"media.next", "下一曲"}, {"media.previous", "上一曲"},
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
#define TGNotificationAppPrefix @"notification.app:" // "notification.app:<bundle id>"
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
#define TGAirPlayListPath TGJB(@"/var/mobile/Library/Preferences/com.johndie.triggr.airplay.plist")

// Settings pages for "Open Settings Page" (App-prefs:<id>, verified to open Settings on iOS 16.7).
static const TGItem TGSettingsPages[] = {
    {"WIFI", "Wi-Fi"}, {"Bluetooth", "蓝牙"}, {"MOBILE_DATA_SETTINGS_ID", "蜂窝网络"},
    {"NOTIFICATIONS_ID", "通知"}, {"Sounds", "声音与触感"}, {"General", "通用"},
    {"ControlCenter", "控制中心"}, {"DISPLAY", "显示与亮度"}, {"Wallpaper", "壁纸"},
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
        @"home.single": @"返回主屏幕", @"home.double": @"应用切换器", @"home.triple": @"辅助功能快捷键",
        @"home.longhold": @"Siri", @"touchid.doubletap": @"便捷访问",
        @"volume.up": @"增大音量", @"volume.down": @"减小音量",
        @"lock.single": @"锁屏", @"lock.longhold": @"关机滑块",
        @"mute.silent": @"静音", @"mute.ring": @"取消静音", @"mute.toggle": @"静音/取消静音",
    }[trigger];
}

// What assigning an action to a trigger does to the button's own action (shown in
// the action picker). Button triggers depend on Replace Button Actions.
static inline NSString *TGTriggerWarning(NSString *trigger, BOOL replaces) {
    NSDictionary *buttons = replaces ? @{
        @"home.single": @"将代替原本返回主屏幕（或唤醒）的操作。若也要返回主屏幕，请同时添加「前往主屏幕」。",
        @"home.double": @"将代替原本的应用切换器。要同时打开它，请添加「应用切换器」。",
        @"touchid.doubletap": @"将代替便捷访问（轻轻点按两下）。要保留它，请添加「便捷访问」。",
        @"home.triple": @"将代替辅助功能快捷键，双击时会稍作等待以确认是否会有第三下。",
        @"home.longhold": @"将代替 Siri。要保留 Siri，请添加「Siri」。",
        @"volume.up": @"将代替增大音量。若也要改变音量，请添加「音量加（媒体）」。",
        @"volume.down": @"将代替减小音量。若也要改变音量，请添加「音量减（媒体）」。",
        @"volume.both": @"按住一个音量键并按下另一个。第二次按下的键不会改变音量；第一个键仍可能微调一步音量。",
        @"volume.bothhold": @"两个音量键同时按住 0.5 秒。第二个键不会改变音量；第一个键仍可能微调一步音量。",
        @"lock.single": @"在屏幕点亮时，将代替原本的锁屏操作。若也要锁屏，请添加「睡眠」。",
        @"lock.double": @"单击会稍作等待，以确认是否会有第二次按动。",
        @"lock.triple": @"单击与双击都会稍作等待，以确认是否会有第三次按动。",
        @"lock.longhold": @"将代替关机滑块。若也要显示它，请添加「关机滑块」。",
        @"mute.silent": @"将代替静音，因此铃声音量保持开启。若也要静音，请添加「关闭静音（开关）」。",
        @"mute.ring": @"将代替取消静音，因此铃声音量保持关闭。若也要取消静音，请添加「开启静音（开关）」。",
        @"mute.toggle": @"将代替原本的静音或取消静音操作。若也要改变静音状态，请添加「切换静音（开关）」。",
    } : @{
        @"home.single": @"与原本的按动一起执行，仍然会返回主屏幕。",
        @"home.double": @"与应用切换器一起执行。",
        @"touchid.doubletap": @"与便捷访问一起执行。",
        @"home.triple": @"与辅助功能快捷键一起执行。",
        @"home.longhold": @"与 Siri 一起执行。",
        @"volume.up": @"与音量变化一起执行。",
        @"volume.down": @"与音量变化一起执行。",
        @"volume.both": @"按住一个音量键并按下另一个。两个键仍会改变音量。",
        @"volume.bothhold": @"两个音量键同时按住 0.5 秒。两个键仍会改变音量。",
        @"lock.single": @"与每次按动一起执行，仍然会锁屏。",
        @"lock.double": @"在连续按动结束后执行；每次按动仍会锁屏或唤醒。",
        @"lock.triple": @"在连续按动结束后执行；每次按动仍会锁屏或唤醒。",
        @"lock.longhold": @"与关机滑块一起执行。",
        @"mute.silent": @"与开关一起执行，仍然会静音。",
        @"mute.ring": @"与开关一起执行，仍然会取消静音。",
        @"mute.toggle": @"与开关一起执行，仍然会切换静音与响铃。",
    };
    if (buttons[trigger]) return replaces ? buttons[trigger] : [buttons[trigger] stringByAppendingString:@" 若要改为代替，请在「选项」中开启「替换按键操作」。"];
    NSDictionary *warnings = @{
        @"home.shorthold": @"按住后在 Siri 出现前松手。",
        @"volume.uphold": @"按住 0.5 秒后执行；音量仍会改变一步。",
        @"volume.downhold": @"按住 0.5 秒后执行；音量仍会改变一步。",
        @"icon.flickleft": @"从图标上开始、向下一页滑动的手势，将执行此操作以代替翻页。在图标之间滑动即可换页。",
        @"icon.flickright": @"从图标上开始、向上一页滑动的手势，将执行此操作以代替翻页。在图标之间滑动即可换页。",
        @"icon.flickdown": @"从图标上开始、向下滑动呼出搜索的手势，将执行此操作以代替搜索。",
        @"volume.updown": @"在半秒内先按音量加、再按音量减。与两次按动一起执行。",
        @"volume.downup": @"在半秒内先按音量减、再按音量加。与两次按动一起执行。",
        @"time": @"在 Triggr 运行时，于该时间点执行。iOS 在手机休眠时可能延迟定时器；若迟到超过 5 分钟则会跳过本次。",
        @"battery": @"在电量经过该百分比时执行一次，而非每次变化都执行。",
        @"statusbar.tap": @"若也分配了双击，单击会稍作等待以确认是否会有第二次点按。",
        @"statusbar.left.tap": @"若也分配了左侧双击（或双击），单击会稍作等待以确认是否会有第二次点按。",
        @"statusbar.right.tap": @"若也分配了右侧双击（或双击），单击会稍作等待以确认是否会有第二次点按。",
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
        TGBatteryAbovePrefix, TGBatteryBelowPrefix, TGAppLaunchedPrefix, TGNotificationAppPrefix, TGTimePrefix,
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
    if ([trigger hasPrefix:TGWiFiJoinedPrefix]) return [NSString stringWithFormat:@"已加入「%@」", value];
    if ([trigger hasPrefix:TGWiFiLeftPrefix]) return [NSString stringWithFormat:@"已离开「%@」", value];
    if ([trigger hasPrefix:TGBTConnectedPrefix]) return [NSString stringWithFormat:@"已连接「%@」", value];
    if ([trigger hasPrefix:TGBTDisconnectedPrefix]) return [NSString stringWithFormat:@"已断开「%@」", value];
    if ([trigger hasPrefix:TGBatteryAbovePrefix]) return [NSString stringWithFormat:@"电量上升到 %@%%", value];
    if ([trigger hasPrefix:TGBatteryBelowPrefix]) return [NSString stringWithFormat:@"电量下降到 %@%%", value];
    if ([trigger hasPrefix:TGAppLaunchedPrefix]) return [@"打开 " stringByAppendingString:value];
    if ([trigger hasPrefix:TGNotificationAppPrefix]) return [@"收到来自 " stringByAppendingString:value];
    if ([trigger hasPrefix:@"icon.flick"]) return [NSString stringWithFormat:@"在 %@ 上%@", value, TGFlickDirectionTitle(trigger)];
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
                if (TGTriggerGroups[g].items == TGIcons) return [NSString stringWithFormat:@"图标 %s", TGTriggerGroups[g].items[i].title];
                if (TGTriggerGroups[g].items == TGHomeGestures) return [NSString stringWithFormat:@"主屏幕 %s", TGTriggerGroups[g].items[i].title];
                return event ? @(TGTriggerGroups[g].items[i].title) : [NSString stringWithFormat:@"%s：%s", TGTriggerGroups[g].title, TGTriggerGroups[g].items[i].title];
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

// A place for one app: "app:<bundle id>" (checked before In Apps and Anywhere).
#define TGAppModePrefix @"app:"

static inline NSString *TGAssignmentKey(NSString *mode, NSString *trigger) {
    return [NSString stringWithFormat:@"%@/%@", mode, trigger];
}

// Action extensions: another package adds a category of actions by installing
// a plist in Library/Triggr/Extensions (see EXTENSIONS.md). Its items are the
// files in a folder; running one runs the extension's program with the item's
// name as its only argument (no shell). Action id: "ext:<extension>:<item>".
#define TGExtensionPrefix @"ext:"
// Where SpringBoard can't start programs (iOS 18), triggrd runs Run Command and
// extension actions: one request plist per run in this queue, then the notification.
#define TGRunQueue @"/var/tmp/com.johndie.triggr-run"
#define TGRunRequestNotification "com.johndie.triggr/run-request"

static inline NSString *TGExtensionsDirectory(void) {
    for (NSString *path in @[TGJB(@"/Library/Triggr/Extensions"), @"/Library/Triggr/Extensions"])
        if ([NSFileManager.defaultManager fileExistsAtPath:path]) return path;
    return nil;
}

// No extensions installed = no Extensions folder: TGExtensionsDirectory() is nil then. Never hand an empty path to
// contentsOfDirectoryAtPath: — NSFileManager throws on "" (1.0.4 crashed the action picker on every install without
// EQELinker; issue #2 by 777qwq found it on RootHide).
static inline NSArray<NSString *> *TGExtensionNames(void) {
    NSMutableArray *names = [NSMutableArray array];
    NSString *dir = TGExtensionsDirectory();
    if (!dir.length) return names;
    for (NSString *file in [NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:nil])
        if ([file.pathExtension isEqualToString:@"plist"]) [names addObject:file.stringByDeletingPathExtension];
    return [names sortedArrayUsingSelector:@selector(localizedStandardCompare:)];
}

// {Title, ItemTitle, Symbol, Color, ItemsDirectory, ItemsExtension, ItemsExclude, Program}
static inline NSDictionary *TGExtension(NSString *name) {
    NSString *dir = TGExtensionsDirectory();
    if (!name.length || [name containsString:@"/"] || !dir.length) return nil;
    NSDictionary *extension = [NSDictionary dictionaryWithContentsOfFile:[[dir stringByAppendingPathComponent:name] stringByAppendingPathExtension:@"plist"]];
    return [extension[@"Title"] isKindOfClass:NSString.class] && [extension[@"Program"] isKindOfClass:NSString.class] ? extension : nil;
}

static inline NSArray<NSString *> *TGExtensionItems(NSDictionary *extension) {
    NSString *directory = extension[@"ItemsDirectory"], *type = extension[@"ItemsExtension"];
    NSArray *exclude = [extension[@"ItemsExclude"] isKindOfClass:NSArray.class] ? extension[@"ItemsExclude"] : @[];
    if (![directory isKindOfClass:NSString.class] || !directory.length) return @[]; // "" would throw too
    NSMutableArray *items = [NSMutableArray array];
    for (NSString *file in [NSFileManager.defaultManager contentsOfDirectoryAtPath:directory error:nil]) {
        if ([type isKindOfClass:NSString.class] && type.length && ![file.pathExtension isEqualToString:type]) continue;
        NSString *item = [type isKindOfClass:NSString.class] && type.length ? file.stringByDeletingPathExtension : file;
        if (item.length && ![item hasPrefix:@"."] && ![exclude containsObject:item]) [items addObject:item];
    }
    return [items sortedArrayUsingSelector:@selector(localizedStandardCompare:)];
}

// "ext:eqe:Bass Boost" -> @[@"eqe", @"Bass Boost"]
static inline NSArray<NSString *> *TGExtensionAction(NSString *action) {
    if (![action hasPrefix:TGExtensionPrefix]) return nil;
    NSString *rest = [action substringFromIndex:TGExtensionPrefix.length];
    NSRange colon = [rest rangeOfString:@":"];
    if (colon.location == NSNotFound || colon.location == 0 || colon.location + 1 >= rest.length) return nil;
    return @[[rest substringToIndex:colon.location], [rest substringFromIndex:colon.location + 1]];
}

// Display title for an action id ("None" when unassigned).
static inline NSString *TGActionTitle(NSString *action) {
    if (action.length == 0) return @"无";
    NSArray *extension = TGExtensionAction(action);
    if (extension) {
        id title = TGExtension(extension[0])[@"ItemTitle"];
        return [NSString stringWithFormat:@"%@: %@", [title isKindOfClass:NSString.class] ? title : extension[0], extension[1]];
    }
    if ([action hasPrefix:TGShortcutPrefix]) return [@"捷径：" stringByAppendingString:[action substringFromIndex:TGShortcutPrefix.length]];
    if ([action hasPrefix:TGURLPrefix]) return [@"URL：" stringByAppendingString:[action substringFromIndex:TGURLPrefix.length]];
    if ([action hasPrefix:TGShellPrefix]) return [@"命令：" stringByAppendingString:[action substringFromIndex:TGShellPrefix.length]];
    if ([action hasPrefix:TGAppPrefix]) return [@"打开 " stringByAppendingString:[action substringFromIndex:TGAppPrefix.length]];
    if ([action hasPrefix:TGPausePrefix]) return [NSString stringWithFormat:@"暂停 %@ 秒", [action substringFromIndex:TGPausePrefix.length]];
    if ([action hasPrefix:TGMenuPrefix]) return @"菜单";
    if ([action hasPrefix:TGBrightnessPrefix]) return [NSString stringWithFormat:@"亮度 %@%%", [action substringFromIndex:TGBrightnessPrefix.length]];
    if ([action hasPrefix:TGMediaVolumePrefix]) return [NSString stringWithFormat:@"媒体音量 %@%%", [action substringFromIndex:TGMediaVolumePrefix.length]];
    if ([action hasPrefix:TGRingerVolumePrefix]) return [NSString stringWithFormat:@"铃声音量 %@%%", [action substringFromIndex:TGRingerVolumePrefix.length]];
    if ([action hasPrefix:TGMessagePrefix]) return [@"消息：" stringByAppendingString:[action substringFromIndex:TGMessagePrefix.length]];
    if ([action hasPrefix:TGSpeakPrefix]) return [@"朗读：" stringByAppendingString:[action substringFromIndex:TGSpeakPrefix.length]];
    if ([action hasPrefix:TGAirPlayPrefix]) return [@"AirPlay 到 " stringByAppendingString:[action substringFromIndex:TGAirPlayPrefix.length]];
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
