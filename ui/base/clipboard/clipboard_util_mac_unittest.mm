// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/clipboard/clipboard_util_mac.h"

#include "base/mac/scoped_nsobject.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/mozilla/NSPasteboard+Utils.h"

namespace {

class ClipboardUtilMacTest : public PlatformTest {
 public:
  ClipboardUtilMacTest() { }
};

TEST_F(ClipboardUtilMacTest, PasteboardItemFromUrl) {
  NSString* urlString =
      @"https://www.google.com/"
      @"search?q=test&oq=test&aqs=chrome..69i57l2j69i60l4.278j0j7&"
      @"sourceid=chrome&ie=UTF-8";

  base::scoped_nsobject<NSPasteboardItem> item(
      ui::ClipboardUtil::PasteboardItemFromUrl(urlString, nil));
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithUniqueName];
  [pasteboard writeObjects:@[ item ]];

  NSArray* urls = nil;
  NSArray* titles = nil;
  [pasteboard getURLs:&urls andTitles:&titles convertingFilenames:NO];

  ASSERT_EQ(1u, [urls count]);
  EXPECT_NSEQ(urlString, [urls objectAtIndex:0]);
  ASSERT_EQ(1u, [titles count]);
  EXPECT_NSEQ(urlString, [titles objectAtIndex:0]);

  NSURL* url = [NSURL URLFromPasteboard:pasteboard];
  EXPECT_NSEQ([url absoluteString], urlString);
}

TEST_F(ClipboardUtilMacTest, PasteboardItemWithTitle) {
  NSString* urlString = @"https://www.google.com/";
  NSString* title = @"Burrowing Yams";

  base::scoped_nsobject<NSPasteboardItem> item(
      ui::ClipboardUtil::PasteboardItemFromUrl(urlString, title));
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithUniqueName];
  [pasteboard writeObjects:@[ item ]];

  NSArray* urls = nil;
  NSArray* titles = nil;
  [pasteboard getURLs:&urls andTitles:&titles convertingFilenames:NO];

  ASSERT_EQ(1u, [urls count]);
  EXPECT_NSEQ(urlString, [urls objectAtIndex:0]);
  ASSERT_EQ(1u, [titles count]);
  EXPECT_NSEQ(title, [titles objectAtIndex:0]);

  NSURL* url = [NSURL URLFromPasteboard:pasteboard];
  EXPECT_NSEQ([url absoluteString], urlString);
}

TEST_F(ClipboardUtilMacTest, PasteboardItemWithFilePath) {
  NSURL* url = [NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES];
  ASSERT_TRUE(url);
  NSString* urlString = [url absoluteString];

  base::scoped_nsobject<NSPasteboardItem> item(
      ui::ClipboardUtil::PasteboardItemFromUrl(urlString, nil));
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithUniqueName];
  [pasteboard writeObjects:@[ item ]];

  NSArray* urls = nil;
  NSArray* titles = nil;
  [pasteboard getURLs:&urls andTitles:&titles convertingFilenames:NO];

  ASSERT_EQ(1u, [urls count]);
  EXPECT_NSEQ(urlString, [urls objectAtIndex:0]);
  ASSERT_EQ(1u, [titles count]);
  EXPECT_NSEQ(urlString, [titles objectAtIndex:0]);

  NSURL* urlFromPasteboard = [NSURL URLFromPasteboard:pasteboard];
  EXPECT_NSEQ(urlFromPasteboard, url);
}

}  // namespace