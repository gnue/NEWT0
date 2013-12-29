//
//  NWViewController.m
//  iNewtExample
//
//  Created by Steve White on 3/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#import "NWViewController.h"
#import "iNewt.h"


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

  newtErr	err;
  newtRefVar result = NVMInterpretFile([file fileSystemRepresentation], &err);
  newt_result_message(result, err);
}

- (void) viewWillAppear:(BOOL)animated {
  [super viewWillAppear:animated];
  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
    iNewt_Setup();
    NSArray *newtScripts = [[NSBundle mainBundle] pathsForResourcesOfType:@"newt" inDirectory:nil];
    for (NSString *aScriptPath in newtScripts) {
      [self interpretNewtFile:aScriptPath];
    }
    NVMInterpretStr("local libc := OpenNativeLibrary(\"libc\");\
                    libc:DefGlobalFn(\
                                     '|libc.printf|,\
                                     {name: \"printf\",\
                                     args: ['string],\
                                     result: 'sint32});\
                    |libc.printf|(\"Hello from NEWT0!\\n\");\
                    libc:Close();\
                    ", NULL);
    iNewt_Cleanup();
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
