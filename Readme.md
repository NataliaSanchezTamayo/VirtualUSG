# VirtualUSG!
VirtualUSG is a Directshow filter named Virtual Cam that reads an bitmap (32 bits per pixel) and renders it to a virtual camera device that can be accessed like a web cam in **VLC, TeamViewer and Skype** applications. It does not yet support **WebRTC** 


## Details
The Direct Show filter creates the library **Cam.dll** that must be registered using regsvr32, the file is located in the Debug (release) folder. The bitmap filed accessed is called Bitmap1.bmp  
Once registered the filter is always accessible as a web camera device, the bitmap is updated when the application b_frame_grabber under the project (Folder) DSCam.


at this point image in the team viewer display appears to be mirrored
path to the file is curently in the desktop, must be changed
camera is not detected with WebRTC test

this is the master branch
This code contains two filters vcam and bitmapsender
However, Only the filter bitmap sender is being registered and unregistered!!
the filter works correctly with splitcam and directshow applications but is not detected as hardware in the computer. So it requires the use of splitcam driver for the web browser to find it as a camera. driver is not setup correctly for team viewer.

##Register
-When using this filter bitmapsender with splitcam you must make sure that the .dll located in the folder: \Cam\Debug is registered using regsvr32, and that .dll from other branches are unregistered!

