// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "electron/browser/mac/atom_application_delegate.h"

#import "electron/browser/mac/atom_application.h"
#include "electron/browser/browser.h"
#include "base/strings/sys_string_conversions.h"

@implementation ElectronApplicationDelegate

- (id)init {
  self = [super init];
  menu_controller_.reset([[ElectronMenuController alloc] init]);
  return self;
}

- (void)setApplicationDockMenu:(ui::MenuModel*)model {
  [menu_controller_ populateWithModel:model];
}

- (void)applicationWillFinishLaunching:(NSNotification*)notify {
  // Don't add the "Enter Full Screen" menu item automatically.
  [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];

  // Add observer to monitor the system's Dark Mode theme.
  [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(platformThemeChanged:) name:@"AppleInterfaceThemeChangedNotification" object:nil];

  electron::Browser::Get()->WillFinishLaunching();
}

- (void)applicationDidFinishLaunching:(NSNotification*)notify {
  electron::Browser::Get()->DidFinishLaunching();
}

- (NSMenu*)applicationDockMenu:(NSApplication*)sender {
  return [menu_controller_ menu];
}

- (BOOL)application:(NSApplication*)sender
           openFile:(NSString*)filename {
  std::string filename_str(base::SysNSStringToUTF8(filename));
  return electron::Browser::Get()->OpenFile(filename_str) ? YES : NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender {
  electron::Browser* browser = electron::Browser::Get();
  if (browser->is_quiting()) {
    return NSTerminateNow;
  } else {
    // System started termination.
    electron::Browser::Get()->Quit();
    return NSTerminateCancel;
  }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication
                    hasVisibleWindows:(BOOL)flag {
  electron::Browser* browser = electron::Browser::Get();
  browser->Activate(static_cast<bool>(flag));
  return flag;
}

- (void)platformThemeChanged:(NSNotification *)notify {
  electron::Browser::Get()->PlatformThemeChanged();
}

@end
