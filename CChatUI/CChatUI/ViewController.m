//
//  ViewController.m
//  CChatUI
//
//  Created by pvereza on 5/24/15.
//  Copyright (c) 2015 pedrovereza. All rights reserved.
//

#import "ViewController.h"
#import "cchat/cchat_client.h"
#include <unistd.h>

char inputBuffer[MESSAGE_LEN];

int fd[2];

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    self.pipe = [NSPipe pipe] ;
    self.pipeReadHandle = [self.pipe fileHandleForReading] ;
    dup2([[self.pipe fileHandleForWriting] fileDescriptor], fileno(stdout)) ;
    
    [[NSNotificationCenter defaultCenter] addObserver: self selector: @selector(handleNotification:) name: NSFileHandleReadCompletionNotification object: self.pipeReadHandle] ;
    [self.pipeReadHandle readInBackgroundAndNotify];
    
    pipe(fd);
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0ul);
    dispatch_async(queue, ^{
        
        run(fd);
        [[NSApplication sharedApplication] terminate:nil];
        
    });
}

- (void) updateConversationWithNSString:(NSString *) string {
    [[self.conversationTextView textStorage] appendAttributedString:[[NSAttributedString alloc] initWithString:[NSString stringWithFormat:@"%@\n", string]]];
}

- (void) handleNotification: (NSNotification *)notification {
    [self.pipeReadHandle readInBackgroundAndNotify] ;
    NSString *str = [[NSString alloc] initWithData: [[notification userInfo] objectForKey: NSFileHandleNotificationDataItem] encoding: NSASCIIStringEncoding];
    
    [self updateConversationWithNSString:str];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];
    
}

- (IBAction)textEntered:(id)sender {
    write(fd[1], [[self.senderTextField stringValue] UTF8String], strlen([[self.senderTextField stringValue] UTF8String]));
    
    if ([[self.senderTextField stringValue] hasPrefix:@"/join"] || [[self.senderTextField stringValue] hasPrefix:@"/leave"]) {
        [[[self.conversationTextView textStorage] mutableString] setString:@""];
    }
    else if (![[self.senderTextField stringValue] hasPrefix:@"/"]) {
        [self updateConversationWithNSString:[NSString stringWithFormat:@"You: %@", [self.senderTextField stringValue]]];
    }
    
    [self.senderTextField setStringValue:@""];
}
@end
