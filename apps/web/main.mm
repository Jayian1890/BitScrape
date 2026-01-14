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
        NSImage *resizedIcon = [self resizeImage:icon toSize:NSMakeSize(18, 18)];
        [resizedIcon setTemplate:YES]; // allow dark mode adaptation
        self.statusItem.button.image = resizedIcon;
    }
    self.statusItem.button.title = @"";
    self.statusItem.menu = [self createMenu];
}

// Manually resize image to ensure it draws at the correct size
- (NSImage *)resizeImage:(NSImage *)sourceImage toSize:(NSSize)size {
    NSImage *newImage = [[NSImage alloc] initWithSize:size];
    [newImage lockFocus];
    [sourceImage drawInRect:NSMakeRect(0, 0, size.width, size.height)
                   fromRect:NSZeroRect
                  operation:NSCompositingOperationCopy
                   fraction:1.0];
    [newImage unlockFocus];
    return newImage;
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
    
    // Add Open Web UI item
    NSMenuItem *openWebItem = [[NSMenuItem alloc] initWithTitle:@"Open Web UI" action:@selector(openWebUI:) keyEquivalent:@""];
    openWebItem.target = self;
    [menu addItem:openWebItem];

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
        // Automatically open Web UI when starting
        [self openWebUI:nil];
    }
    [self updateToggleTitle];
}

- (void)openWebUI:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://localhost:8080"]];
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
