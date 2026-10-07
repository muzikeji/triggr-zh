// triggr: run Triggr actions, triggers and menus from the command line.
// It only posts the API's Darwin notifications; SpringBoard decides what runs
// (and nothing does unless "Allow API" is on in Settings → Triggr).

#import <Foundation/Foundation.h>
#import <notify.h>
#import "../Shared/TGCatalog.h"

// Triggr's settings live in the mobile user's domain, whoever runs this.
static id TGSetting(NSString *key) {
    return CFBridgingRelease(CFPreferencesCopyValue((__bridge CFStringRef)key, (__bridge CFStringRef)TGDomain, CFSTR("mobile"), kCFPreferencesAnyHost));
}

static int TGUsage(void) {
    fprintf(stderr,
        "用法: triggr run <操作>        运行一个内置操作（参见: triggr list）\n"
        "      triggr trigger <触发器>  运行分配给某触发器的操作\n"
        "      triggr menu <名称>       显示你的一个菜单\n"
        "      triggr list              列出所有操作和触发器\n");
    return 64;
}

static void TGPrint(const TGGroup *groups, int count) {
    for (int g = 0; g < count; g++) {
        printf("%s\n", groups[g].title);
        for (int i = 0; i < groups[g].count; i++) printf("  %-22s %s\n", groups[g].items[i].identifier, groups[g].items[i].title);
    }
}

static int TGPost(NSString *name) {
    uint32_t status = notify_post([@TGAPIPrefix stringByAppendingString:name].UTF8String);
    if (status != NOTIFY_STATUS_OK) {
        fprintf(stderr, "triggr: 无法发送（notify 状态 %u）\n", status);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    @autoreleasepool {
        if (argc < 2) return TGUsage();
        NSString *command = @(argv[1]);
        if ([command isEqualToString:@"list"]) {
            printf("操作（triggr run <操作>）:\n");
            TGPrint(TGActionGroups, TG_COUNT(TGActionGroups));
            printf("\n触发器（triggr trigger <触发器>; 仅在已分配时运行）:\n");
            TGPrint(TGTriggerGroups, TG_COUNT(TGTriggerGroups));
            return 0;
        }
        if (argc < 3) return TGUsage();
        NSString *value = @(argv[2]);
        if (![TGSetting(TGAllowAPIKey) boolValue]) {
            fprintf(stderr, "triggr: 请先在 设置 → Triggr → 允许 API 中开启此功能。\n");
            return 1;
        }
        if ([command isEqualToString:@"run"]) {
            BOOL known = NO;
            for (int g = 0; g < TG_COUNT(TGActionGroups); g++)
                for (int i = 0; i < TGActionGroups[g].count; i++)
                    if ([value isEqualToString:@(TGActionGroups[g].items[i].identifier)]) known = YES;
            if (!known) {
                fprintf(stderr, "triggr: 未知操作 '%s'（参见: triggr list）\n", argv[2]);
                return 1;
            }
            return TGPost([@"run/" stringByAppendingString:value]);
        }
        if ([command isEqualToString:@"trigger"]) {
            if (!TGIsKnownTrigger(value)) {
                fprintf(stderr, "triggr: 未知触发器 '%s'（参见: triggr list）\n", argv[2]);
                return 1;
            }
            return TGPost([@"trigger/" stringByAppendingString:value]);
        }
        if ([command isEqualToString:@"menu"]) {
            id menus = TGSetting(TGMenusKey);
            for (id menu in [menus isKindOfClass:NSArray.class] ? menus : @[])
                if ([menu isKindOfClass:NSDictionary.class] && [[menu[@"name"] description] caseInsensitiveCompare:value] == NSOrderedSame)
                    return TGPost([@"menu/" stringByAppendingString:[menu[@"id"] description]]);
            fprintf(stderr, "triggr: 不存在名为 '%s' 的菜单\n", argv[2]);
            return 1;
        }
        return TGUsage();
    }
}
