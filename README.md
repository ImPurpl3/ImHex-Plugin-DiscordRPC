# ImHex v.1.37.4 Discord Rich Presence

Ths Plugin adds Discord Rich Presence support to ImHex v.1.37.4

## Notes
- My build CI does not build for Mac because it was faster without it
  - If you want to build for Mac, fork and use the original build CI but change the SDK version to 1.37.4 and change the checkout version to v4
- I'd also only recommend having GitHub host the build system because self hosting is difficult on Windows with msys2
- This works for ImHex v1.37.4 and most likely only this version! The dev has stated that older plugins likely won't work on newer versions
- The settings page probably doesn't work
  - I wrote this update mostly using Copilot (and completely on GitHub in the browser) because I have very little C++ experience so while I have a rough idea of whats going on with the settings checkboxes, I have no idea if the checkboxes properly toggle the RPC status and details

![image](https://user-images.githubusercontent.com/10835354/211030126-37ea5d52-07e3-468b-82dc-18ccf971f128.png)
