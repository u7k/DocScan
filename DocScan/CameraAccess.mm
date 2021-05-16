//
//  CameraAccess.m
//  DocScan
//
//  Created by Uygur Kıran on 16.05.2021.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "CameraAccess.h"

bool CameraAccess::isGranted() {
    AVAuthorizationStatus st = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (st == AVAuthorizationStatusAuthorized) {
        return true;
    }
    
    dispatch_group_t group = dispatch_group_create();
    
    __block bool accessGranted = false;
    
    if (st != AVAuthorizationStatusAuthorized) {
        dispatch_group_enter(group);
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
            
            accessGranted = granted;
            dispatch_group_leave(group);
        }];
    }
    
    dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(5.0 * NSEC_PER_SEC)));
    
    return accessGranted;
}
