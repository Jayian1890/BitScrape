#import <Cocoa/Cocoa.h>
#include "bitscrape/web/menubar_controller.hpp"

using namespace bitscrape::web;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSStatusItem *statusItem;
@end

@implementation AppDelegate {
    // C++ controller pointer
    MenubarController *controller_; // raw pointer managed manually
    BOOL serverRunning_;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    // Hide dock icon (make app an agent)
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

    controller_ = new MenubarController();
    serverRunning_ = NO;

    // Create status item
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
    NSImage *icon = [NSImage imageNamed:@"menubar_icon"];
    if (icon) {
        [icon setTemplate:YES]; // allow dark mode adaptation
        self.statusItem.button.image = icon;
    }
    self.statusItem.button.title = @"";
    self.statusItem.menu = [self createMenu];
}

- (void)dealloc {
    if (controller_) {
        delete controller_;
        controller_ = nullptr;
    }
    [super dealloc];
}

- (NSMenu *)createMenu {
    NSMenu *menu = [[NSMenu alloc] initWithTitle:@"BitScrape"];
    NSMenuItem *toggleItem = [[NSMenuItem alloc] initWithTitle:@"Start Server" action:@selector(toggleServer:) keyEquivalent:@""];
    toggleItem.target = self;
    [menu addItem:toggleItem];
    [menu addItem:[NSMenuItem separatorItem]];
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Exit" action:@selector(quitApp:) keyEquivalent:@"q"];
    quitItem.target = self;
    [menu addItem:quitItem];
    return menu;
}

- (void)updateToggleTitle {
    NSMenuItem *item = self.statusItem.menu.itemArray[0]; // first item is toggle
    if (serverRunning_) {
        item.title = @"Stop Server";
    } else {
        item.title = @"Start Server";
    }
}

- (void)toggleServer:(id)sender {
    if (serverRunning_) {
        controller_->stop_server();
        serverRunning_ = NO;
    } else {
        serverRunning_ = controller_->start_server(8080);
    }
    [self updateToggleTitle];
}

- (void)quitApp:(id)sender {
    [NSApp terminate:nil];
}

@end

int main(int argc, const char * argv[]) {
    (void)argc;
    (void)argv;
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}
