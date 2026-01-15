#import <Cocoa/Cocoa.h>
#include "bitscrape/web/menubar_controller.hpp"

using namespace bitscrape::web;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSStatusItem *statusItem;
@end

// Menubar icon size
#define ICON_SIZE 18

// Server port
#define SERVER_PORT 8080

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
        NSImage *resizedIcon = [self resizeImage:icon toSize:NSMakeSize(ICON_SIZE, ICON_SIZE)];
        [resizedIcon setTemplate:NO];
        self.statusItem.button.image = resizedIcon;
    }
    self.statusItem.button.title = @"";
    self.statusItem.menu = [self createMenu];

    // Auto-start server and crawler on launch
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    serverRunning_ = controller_->start_server(SERVER_PORT, [resourcePath UTF8String]);
    [self updateStatusLine];
    
    // Automatically open Web UI on startup
    if (serverRunning_) {
        [self openWebUI:nil];
    }
}

// Manually resize image to ensure it draws at the correct size
- (NSImage *)resizeImage:(NSImage *)sourceImage toSize:(NSSize)size {
    NSImage *newImage = [[NSImage alloc] initWithSize:size];
    [newImage lockFocus];
    [[NSColor clearColor] set];
    NSRectFill(NSMakeRect(0, 0, size.width, size.height));
    [sourceImage drawInRect:NSMakeRect(0, 0, size.width, size.height)
                   fromRect:NSZeroRect
                  operation:NSCompositingOperationSourceOver
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
    
    // Status line (not clickable)
    NSMenuItem *statusItem = [[NSMenuItem alloc] initWithTitle:@"Status: Stopped" action:nil keyEquivalent:@""];
    statusItem.enabled = NO;
    [menu addItem:statusItem];
    
    [menu addItem:[NSMenuItem separatorItem]];
    
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

- (void)updateStatusLine {
    NSMenuItem *item = self.statusItem.menu.itemArray[0]; // first item is status
    if (serverRunning_) {
        item.title = @"Status: Running";
    } else {
        item.title = @"Status: Stopped";
    }
}

- (void)openWebUI:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithFormat:@"http://localhost:%d", SERVER_PORT]]];
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
