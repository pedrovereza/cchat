//
//  ViewController.h
//  CChatUI
//
//  Created by pvereza on 5/24/15.
//  Copyright (c) 2015 pedrovereza. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface ViewController : NSViewController

@property (strong) NSPipe *pipe;
@property (strong) NSFileHandle *pipeReadHandle;

@property (weak) IBOutlet NSTextField *senderTextField;
@property (unsafe_unretained) IBOutlet NSTextView *conversationTextView;

- (IBAction)textEntered:(id)sender;


@end

