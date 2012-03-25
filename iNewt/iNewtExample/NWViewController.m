//
//  NWViewController.m
//  iNewtExample
//
//  Created by Steve White on 3/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "NWViewController.h"

#import "NewtCore.h"
#import "NewtParser.h"
#import "NewtVM.h"

@interface NWViewController ()

@end

void yyerror(char * s)
{
	if (s[0] && s[1] == ':')
		NPSErrorStr(*s, s + 2);
	else
		NPSErrorStr('E', s);
}

void newt_result_message(newtRefArg r, newtErr err)
{
  if (err != kNErrNone)
    NewtErrMessage(err);
  else if (NEWT_DEBUG)
    NsPrint(kNewtRefNIL, r);
}

@implementation NWViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
}

- (void) interpretNewtFile:(NSString *)file {
  NSLog(@"%s %@", __PRETTY_FUNCTION__, file);

  
  NewtInit(0, NULL, 0);
  newtErr	err;
  newtRefVar result = NVMInterpretFile([file fileSystemRepresentation], &err);
  newt_result_message(result, err);
  NewtCleanup();
}

- (void) viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    NSArray *newtScripts = [[NSBundle mainBundle] pathsForResourcesOfType:@"newt" inDirectory:nil];
    for (NSString *aScriptPath in newtScripts) {
      [self interpretNewtFile:aScriptPath];
    }
  });
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
  if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
      return (interfaceOrientation != UIInterfaceOrientationPortraitUpsideDown);
  } else {
      return YES;
  }
}

@end
